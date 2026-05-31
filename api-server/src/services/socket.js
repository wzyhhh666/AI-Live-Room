const roomService = require('./room')
const db = require('./database')
const giftService = require('./gift')
const rateLimiter = require('./rateLimiter')
const streamService = require('./streamService')
const Redis = require('ioredis')

const redisPublisher = new Redis({
  host: process.env.REDIS_HOST || '127.0.0.1',
  port: parseInt(process.env.REDIS_PORT || '6379'),
  lazyConnect: true
})
redisPublisher.connect().catch(err => console.error('socket.js Redis publisher connect error:', err.message))

module.exports = function(io) {
  io.on('connection', async (socket) => {
    console.log(`🔌 User connected: ${socket.id}`)
    
    socket.on('join-room', async ({ roomId, userInfo }) => {
      try {
        console.log(`[join-room] RECEIVED from ${socket.id}: roomId=${roomId}, hasUserInfo=${!!userInfo}, userId=${userInfo?.userId}, username=${userInfo?.username}`)
        
        socket.join(`room-${roomId}`)
        
        socket.data.roomId = roomId
        socket.data.userInfo = userInfo

        if (userInfo?.userId && userInfo?.username) {
          await roomService.updateRoomMember(roomId, userInfo.userId, userInfo.username)
        }
        
        const roomInfo = await roomService.getRoomInfo(roomId)
        if (roomInfo.success) {
          socket.emit('room-info', roomInfo.data)
          socket.data.isHost = (userInfo?.userId && roomInfo.data.hostId === userInfo.userId)
        }

        await roomService.updateOnlineCount(roomId, 1)

        const onlineUsers = await io.in(`room-${roomId}`).fetchSockets()
        io.to(`room-${roomId}`).emit('online-count', {
          count: onlineUsers.length,
          roomId
        })

        socket.emit('joined-room', {
          success: true,
          roomId,
          onlineUsers: onlineUsers.length
        })

        try {
          const rankResult = await giftService.getGiftRank(parseInt(roomId))
          // #region debug-point A:join-rank
          fetch("http://127.0.0.1:7778/event",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({sessionId:"gift-rank-empty",runId:"pre",hypothesisId:"A",location:"socket.js:52",msg:"[DEBUG] join-room rankResult",data:{roomId,success:rankResult.success,dataLength:rankResult.data?.length||0}})}).catch(()=>{});
          // #endregion
          if (rankResult.success) {
            socket.emit('gift-rank-update', rankResult.data)
            // #region debug-point A:join-emit
            fetch("http://127.0.0.1:7778/event",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({sessionId:"gift-rank-empty",runId:"pre",hypothesisId:"A",location:"socket.js:56",msg:"[DEBUG] join-room emitted gift-rank-update",data:{roomId,socketId:socket.id}})}).catch(()=>{});
            // #endregion
          }
        } catch (rankError) {
          // #region debug-point A:join-error
          fetch("http://127.0.0.1:7778/event",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({sessionId:"gift-rank-empty",runId:"pre",hypothesisId:"A",location:"socket.js:59",msg:"[DEBUG] join-room ranking ERROR",data:{roomId,error:rankError.message}})}).catch(()=>{});
          // #endregion
        }
        
        console.log(`👤 ${userInfo?.username || 'Anonymous'} joined room ${roomId}`)

      } catch (error) {
        console.error('Join room error:', error)
        socket.emit('error', { message: 'Failed to join room' })
      }
    })
    
    socket.on('send-danmaku', async ({ roomId, content, color }) => {
      try {
        if (!socket.data.userInfo) {
          socket.emit('error', { message: 'Not authenticated' })
          return
        }
        
        const { userId, username } = socket.data.userInfo

        const limitResult = await rateLimiter.checkDanmakuLimit(userId, roomId)
        if (!limitResult.allowed) {
          socket.emit('error', { message: limitResult.reason })
          return
        }

        const resolvedColor = color || '#00ff41'
        const now = new Date()

        const pool = await db.getMysqlPool()
        const [result] = await pool.execute(
          `INSERT INTO danmaku_messages (room_id, user_id, username, content, color, type, created_at)
           VALUES (?, ?, ?, ?, ?, 'normal', NOW())`,
          [roomId, userId, username, content, resolvedColor]
        )

        const message = {
          id: result.insertId,
          room_id: parseInt(roomId),
          userId,
          username,
          content,
          color: resolvedColor,
          time: now.toTimeString().slice(0, 8),
          type: 'normal',
          timestamp: Date.now()
        }

        io.to(`room-${roomId}`).emit('new-danmaku', message)

        await redisPublisher.publish('danmaku:input', JSON.stringify({
          room_id: parseInt(roomId),
          user_id: userId,
          username,
          content,
          color: resolvedColor,
          timestamp: Date.now()
        }))
      } catch (error) {
        console.error('Send danmaku error:', error)
        socket.emit('error', { message: 'Failed to send danmaku' })
      }
    })
    
    socket.on('typing', ({ roomId, isTyping }) => {
      socket.to(`room-${roomId}`).emit('user-typing', {
        username: socket.data.userInfo?.username,
        isTyping
      })
    })
    
    socket.on('send-like', async ({ roomId }) => {
      try {
        const result = await roomService.incrementLikeCount(roomId)
        if (result.success) {
          io.to(`room-${roomId}`).emit('stats-update', {
            roomId,
            likeDelta: 1,
            likeCount: result.data.likeCount
          })
        }
      } catch (error) {
        console.error('Send like error:', error)
      }
    })
    
    socket.on('send-gift', async ({ roomId, giftId, giftName, count, price, effectType }) => {
      try {
        console.log(`[send-gift] RECEIVED from ${socket.id}: roomId=${roomId}, giftName=${giftName}, hasUserInfo=${!!socket.data.userInfo}, isHost=${socket.data.isHost}`)
        
        if (!socket.data.userInfo) {
          console.log(`[send-gift] REJECTED: no userInfo for ${socket.id}`)
          socket.emit('error', { message: 'Not authenticated' })
          return
        }

        if (socket.data.isHost) {
          socket.emit('error', { message: '主播不能给自己送礼物' })
          return
        }

        const { userId, username } = socket.data.userInfo

        const giftCount = parseInt(count) || 1
        const giftPrice = parseFloat(price) || 0
        const resolvedGiftName = giftName || '礼物'
        const resolvedEffect = effectType || 'normal'

        const pool = await db.getMysqlPool()
        const [roomRows] = await pool.execute('SELECT host_id FROM rooms WHERE id = ?', [roomId])
        const receiverId = roomRows.length > 0 ? roomRows[0].host_id : 0

        await pool.execute(
          `INSERT INTO gift_record (room_id, sender_id, sender_name, receiver_id, gift_id, gift_name, gift_count, total_price, effect_type)
           VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)`,
          [parseInt(roomId), userId, username, receiverId, parseInt(giftId) || 0, resolvedGiftName, giftCount, giftPrice, resolvedEffect]
        )
        // #region debug-point C:gift-inserted
        fetch("http://127.0.0.1:7778/event",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({sessionId:"gift-rank-empty",runId:"pre",hypothesisId:"C",location:"socket.js:163",msg:"[DEBUG] send-gift INSERTED gift_record",data:{roomId,sender:username,gift:resolvedGiftName,price:giftPrice}})}).catch(()=>{});
        // #endregion

        const giftEvent = {
          room_id: parseInt(roomId),
          senderId: userId,
          senderName: username,
          giftId: parseInt(giftId) || 0,
          giftName: resolvedGiftName,
          count: giftCount,
          totalPrice: giftPrice,
          effectType: resolvedEffect,
          timestamp: Date.now()
        }

        io.to(`room-${roomId}`).emit('new-gift', giftEvent)

        await redisPublisher.publish('gift:input', JSON.stringify({
          room_id: parseInt(roomId),
          sender_id: userId,
          sender_name: username,
          gift_id: parseInt(giftId) || 0,
          gift_name: resolvedGiftName,
          gift_count: giftCount,
          total_price: giftPrice,
          effect_type: resolvedEffect,
          timestamp: Date.now()
        }))

        try {
          await redisPublisher.zincrby(`rank:room:${roomId}:gift`, giftPrice, `${userId}:${username}`)
          const rankResult = await giftService.getGiftRank(parseInt(roomId))
          // #region debug-point C:rank-result
          fetch("http://127.0.0.1:7778/event",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({sessionId:"gift-rank-empty",runId:"pre",hypothesisId:"C",location:"socket.js:198",msg:"[DEBUG] send-gift rankResult",data:{roomId,success:rankResult.success,dataLength:rankResult.data?.length||0}})}).catch(()=>{});
          // #endregion
          if (rankResult.success) {
            io.to(`room-${roomId}`).emit('gift-rank-update', rankResult.data)
            // #region debug-point C:rank-broadcast
            fetch("http://127.0.0.1:7778/event",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({sessionId:"gift-rank-empty",runId:"pre",hypothesisId:"C",location:"socket.js:202",msg:"[DEBUG] send-gift broadcasted gift-rank-update",data:{roomId}})}).catch(()=>{});
            // #endregion
          }
        } catch (rankError) {
          // #region debug-point C:rank-error
          fetch("http://127.0.0.1:7778/event",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({sessionId:"gift-rank-empty",runId:"pre",hypothesisId:"C",location:"socket.js:206",msg:"[DEBUG] send-gift ranking update ERROR",data:{roomId,error:rankError.message}})}).catch(()=>{});
          // #endregion
        }
      } catch (error) {
        console.error('Send gift error:', error)
        socket.emit('error', { message: 'Failed to send gift' })
      }
    })
    
    socket.on('disconnect', async () => {
      console.log(`🔌 User disconnected: ${socket.id}`)
      
      if (socket.data.roomId) {
        await roomService.updateOnlineCount(socket.data.roomId, -1)
        
        const onlineUsers = await io.in(`room-${socket.data.roomId}`).fetchSockets()
        io.to(`room-${socket.data.roomId}`).emit('online-count', {
          count: onlineUsers.length,
          roomId: socket.data.roomId
        })

        if (socket.data.isHost) {
          console.log(`🛑 Host of room ${socket.data.roomId} disconnected, stopping stream`)
          try {
            await streamService.stopStream(socket.data.roomId)
            io.to(`room-${socket.data.roomId}`).emit('stream-ended', {
              roomId: socket.data.roomId,
              timestamp: Date.now()
            })
            io.to(`room-${socket.data.roomId}`).emit('room-closed', {
              roomId: socket.data.roomId
            })
          } catch (err) {
            console.error('Failed to stop stream on host disconnect:', err.message)
          }
        }
      }
    })
    
    socket.on('ping', () => {
      socket.emit('pong', { timestamp: Date.now() })
    })
  })
}
