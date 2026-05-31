const { v4: uuidv4 } = require('uuid')
const db = require('./database')

async function login(username) {
  const pool = await db.getMysqlPool()
  
  try {
    const [users] = await pool.execute(
      'SELECT * FROM users WHERE username = ?',
      [username]
    )
    
    let user
    if (users.length === 0) {
      const token = `token_${uuidv4()}`
      const [result] = await pool.execute(
        `INSERT INTO users (username, nickname, avatar, token) 
         VALUES (?, ?, ?, ?)`,
        [username, username, `https://api.dicebear.com/7.x/avataaars/svg?seed=${username}`, token]
      )
      
      user = {
        userId: result.insertId,
        username,
        nickname: username,
        avatar: `https://api.dicebear.com/7.x/avataaars/svg?seed=${username}`,
        role: 0,
        token
      }
    } else {
      user = users[0]
      const token = `token_${uuidv4()}`
      await pool.execute(
        'UPDATE users SET token = ?, updated_at = NOW() WHERE id = ?',
        [token, user.id]
      )
      user.token = token
    }
    
    return {
      success: true,
      data: {
        userId: user.userId || user.id,
        username: user.username,
        nickname: user.nickname,
        avatar: user.avatar,
        role: user.role,
        token: user.token
      }
    }
  } catch (error) {
    console.error('Login service error:', error)
    throw error
  }
}

async function getUserByToken(token) {
  const pool = await db.getMysqlPool()
  
  try {
    const [users] = await pool.execute(
      'SELECT id, username, nickname, avatar, role FROM users WHERE token = ?',
      [token]
    )
    
    if (users.length === 0) {
      return null
    }
    
    return users[0]
  } catch (error) {
    console.error('Get user error:', error)
    throw error
  }
}

module.exports = {
  login,
  getUserByToken
}
