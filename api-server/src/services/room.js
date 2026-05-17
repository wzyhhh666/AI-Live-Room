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
    const [messages] = await pool.execute(
      `SELECT id, user_id as userId, username, content, color, type, 
              DATE_FORMAT(created_at, '%H:%i:%s') as time 
       FROM danmaku_messages 
       WHERE room_id = ? 
       ORDER BY created_at DESC 
       LIMIT ?`,
      [roomId, limit]
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
    
    await pool.execute(
      'UPDATE rooms SET online_count = online_count + 1 WHERE id = ?',
      [roomId]
    )
    
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

module.exports = {
  getRoomInfo,
  getRecentDanmaku,
  sendDanmaku,
  updateOnlineCount
}
