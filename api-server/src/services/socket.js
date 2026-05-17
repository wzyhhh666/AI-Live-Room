const roomService = require('./room')
const db = require('./database')

module.exports = function(io) {
  io.on('connection', async (socket) => {
    console.log(`🔌 User connected: ${socket.id}`)
    
    socket.on('join-room', async ({ roomId, userInfo }) => {
      try {
        socket.join(`room-${roomId}`)
        
        socket.data.roomId = roomId
        socket.data.userInfo = userInfo
        
        const roomInfo = await roomService.getRoomInfo(roomId)
        if (roomInfo.success) {
          socket.emit('room-info', roomInfo.data)
        }
        
        const recentDanmaku = await roomService.getRecentDanmaku(roomId, 20)
        if (recentDanmaku.success) {
          socket.emit('recent-danmaku', recentDanmaku.data)
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
        
        console.log(`👤 ${userInfo?.username || 'Anonymous'} joined room ${roomId}`)
      } catch (error) {
        console.error('Join room error:', error)
        socket.emit('error', { message: 'Failed to join room' })
      }
    })
    
    socket.on('send-danmaku', async ({ roomId, content }) => {
      try {
        if (!socket.data.userInfo) {
          socket.emit('error', { message: 'Not authenticated' })
          return
        }
        
        const { userId, username } = socket.data.userInfo
        
        const result = await roomService.sendDanmaku(
          roomId, 
          content, 
          userId, 
          username
        )
        
        if (result.success) {
          io.to(`room-${roomId}`).emit('new-danmaku', result.data)
          
          const likeCount = Math.floor(Math.random() * 10)
          io.to(`room-${roomId}`).emit('stats-update', {
            likeDelta: likeCount,
            timestamp: Date.now()
          })
        }
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
    
    socket.on('disconnect', async () => {
      console.log(`🔌 User disconnected: ${socket.id}`)
      
      if (socket.data.roomId) {
        await roomService.updateOnlineCount(socket.data.roomId, -1)
        
        const onlineUsers = await io.in(`room-${socket.data.roomId}`).fetchSockets()
        io.to(`room-${socket.data.roomId}`).emit('online-count', {
          count: onlineUsers.length,
          roomId: socket.data.roomId
        })
      }
    })
    
    socket.on('ping', () => {
      socket.emit('pong', { timestamp: Date.now() })
    })
  })
}
