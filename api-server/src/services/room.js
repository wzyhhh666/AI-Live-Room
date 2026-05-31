const db = require('./database')

const COLORS = ['#00ff41', '#ff006e', '#8338ec', '#3a86ff', '#ffbe0b']

function getRandomColor() {
  return COLORS[Math.floor(Math.random() * COLORS.length)]
}

async function getRoomInfo(roomId) {
  const pool = await db.getMysqlPool()
  
  try {
    const [rooms] = await pool.execute(
      'SELECT * FROM rooms WHERE id = ?',
      [roomId]
    )
    
    if (rooms.length === 0) {
      return { success: false, error: 'Room not found' }
    }
    
    const room = rooms[0]
    
    return {
      success: true,
      data: {
        roomId: room.id,
        roomName: room.room_name,
        hostId: room.host_id,
        hostName: room.host_name,
        title: room.title,
        coverImage: room.cover_image,
        onlineCount: room.online_count,
        likeCount: room.like_count || 0,
        state: room.state
      }
    }
  } catch (error) {
    console.error('Get room info error:', error)
    throw error
  }
}

async function getRecentDanmaku(roomId, limit = 50) {
  const pool = await db.getMysqlPool()
  
  try {
    const danmakuLimit = parseInt(limit)
    const [messages] = await pool.execute(
      `SELECT id, user_id as userId, username, content, color, type, 
              DATE_FORMAT(created_at, '%H:%i:%s') as time 
       FROM danmaku_messages 
       WHERE room_id = ? 
       ORDER BY created_at DESC 
       LIMIT ${danmakuLimit}`,
      [roomId]
    )
    
    return {
      success: true,
      data: messages.reverse(),
      total: messages.length
    }
  } catch (error) {
    console.error('Get recent danmaku error:', error)
    throw error
  }
}

async function sendDanmaku(roomId, content, userId, username) {
  const pool = await db.getMysqlPool()
  const redis = await db.getRedisClient()
  
  try {
    const color = getRandomColor()
    const now = new Date()
    
    const [result] = await pool.execute(
      `INSERT INTO danmaku_messages (room_id, user_id, username, content, color, type, created_at)
       VALUES (?, ?, ?, ?, ?, 'normal', NOW())`,
      [roomId, userId, username, content, color]
    )
    
    const message = {
      id: result.insertId,
      roomId: parseInt(roomId),
      userId,
      username,
      content,
      color,
      time: now.toTimeString().slice(0, 8),
      type: 'normal'
    }
    
    await redis.publish(`room:${roomId}:danmaku`, JSON.stringify(message))
    
    return {
      success: true,
      data: message
    }
  } catch (error) {
    console.error('Send danmaku error:', error)
    throw error
  }
}

async function updateOnlineCount(roomId, delta = 1) {
  const pool = await db.getMysqlPool()
  
  try {
    await pool.execute(
      'UPDATE rooms SET online_count = GREATEST(0, online_count + ?) WHERE id = ?',
      [delta, roomId]
    )
  } catch (error) {
    console.error('Update online count error:', error)
    throw error
  }
}

async function getRoomList(page = 1, pageSize = 20) {
  const pool = await db.getMysqlPool()

  try {
    const offset = (page - 1) * pageSize

    const [[{ total }]] = await pool.execute(
      'SELECT COUNT(*) as total FROM rooms WHERE state = \'living\''
    )

    const limit = parseInt(pageSize)
    const off = parseInt(offset)
    const [rooms] = await pool.execute(
      `SELECT * FROM rooms WHERE state = 'living' ORDER BY online_count DESC LIMIT ${limit} OFFSET ${off}`
    )

    return {
      success: true,
      data: {
        rooms: rooms.map(room => ({
          roomId: room.id,
          roomName: room.room_name,
          hostId: room.host_id,
          hostName: room.host_name,
          title: room.title,
          coverImage: room.cover_image,
          onlineCount: room.online_count,
          state: room.state
        })),
        total,
        page: parseInt(page),
        pageSize: parseInt(pageSize)
      }
    }
  } catch (error) {
    console.error('Get room list error:', error)
    return { success: false, error: 'Failed to get room list' }
  }
}

async function createRoom(roomName, hostId, hostName, title, coverImage) {
  const pool = await db.getMysqlPool()

  try {
    const [result] = await pool.execute(
      `INSERT INTO rooms (room_name, host_id, host_name, title, cover_image, online_count, state)
       VALUES (?, ?, ?, ?, ?, 0, 'idle')`,
      [roomName, hostId, hostName, title, coverImage || null]
    )

    return {
      success: true,
      data: {
        roomId: result.insertId,
        roomName,
        hostId,
        hostName,
        title,
        coverImage: coverImage || null,
        onlineCount: 0,
        state: 'idle'
      }
    }
  } catch (error) {
    console.error('Create room error:', error)
    return { success: false, error: 'Failed to create room' }
  }
}

async function closeRoom(roomId, hostId) {
  const pool = await db.getMysqlPool()

  try {
    const [rooms] = await pool.execute(
      'SELECT host_id FROM rooms WHERE id = ?',
      [roomId]
    )

    if (rooms.length === 0) {
      return { success: false, error: 'Room not found' }
    }

    if (rooms[0].host_id !== hostId) {
      return { success: false, error: 'Not the host' }
    }

    await pool.execute(
      'UPDATE rooms SET state = \'closed\' WHERE id = ?',
      [roomId]
    )

    return { success: true }
  } catch (error) {
    console.error('Close room error:', error)
    return { success: false, error: 'Failed to close room' }
  }
}

async function updateRoomMember(roomId, userId, username) {
  const pool = await db.getMysqlPool()

  try {
    await pool.execute(
      `INSERT INTO room_members (room_id, user_id, username, join_time, leave_time)
       VALUES (?, ?, ?, NOW(), NULL)
       ON DUPLICATE KEY UPDATE join_time = VALUES(join_time), leave_time = NULL, username = VALUES(username)`,
      [roomId, userId, username]
    )

    return { success: true }
  } catch (error) {
    if (error.code === 'ER_NO_REFERENCED_ROW_2' || error.errno === 1452) {
      return { success: false }
    }
    console.error('Update room member error:', error)
    return { success: false }
  }
}

async function incrementLikeCount(roomId, delta = 1) {
  const pool = await db.getMysqlPool()
  
  try {
    await pool.execute(
      'UPDATE rooms SET like_count = GREATEST(0, like_count + ?) WHERE id = ?',
      [delta, roomId]
    )
    
    const [rows] = await pool.execute('SELECT like_count FROM rooms WHERE id = ?', [roomId])
    return { success: true, data: { likeCount: rows[0]?.like_count || 0 } }
  } catch (error) {
    console.error('Increment like count error:', error)
    return { success: false, error: error.message }
  }
}

module.exports = {
  getRoomInfo,
  getRecentDanmaku,
  sendDanmaku,
  updateOnlineCount,
  getRoomList,
  createRoom,
  closeRoom,
  updateRoomMember,
  incrementLikeCount
}
