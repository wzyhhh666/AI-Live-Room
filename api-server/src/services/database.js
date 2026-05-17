const mysql = require('mysql2/promise')
const Redis = require('ioredis')
const config = require('../../config/database')

let mysqlPool = null
let redisClient = null

async function getMysqlPool() {
  if (!mysqlPool) {
    mysqlPool = mysql.createPool(config.database.mysql)
    console.log('✅ MySQL pool created')
  }
  return mysqlPool
}

async function getRedisClient() {
  if (!redisClient) {
    redisClient = new Redis(config.database.redis)
    redisClient.on('connect', () => console.log('✅ Redis connected'))
    redisClient.on('error', (err) => console.error('❌ Redis error:', err))
  }
  return redisClient
}

async function initDatabase() {
  try {
    const pool = await getMysqlPool()
    
    await pool.execute(`
      CREATE TABLE IF NOT EXISTS users (
        id INT AUTO_INCREMENT PRIMARY KEY,
        username VARCHAR(50) NOT NULL UNIQUE,
        nickname VARCHAR(100),
        avatar VARCHAR(255),
        role TINYINT DEFAULT 0 COMMENT '0=观众, 1=主播, 2=管理员',
        token VARCHAR(255),
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
      ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    `)
    
    await pool.execute(`
      CREATE TABLE IF NOT EXISTS rooms (
        id INT AUTO_INCREMENT PRIMARY KEY,
        room_name VARCHAR(100) NOT NULL,
        host_id INT,
        host_name VARCHAR(100),
        title VARCHAR(255),
        cover_image VARCHAR(255),
        online_count INT DEFAULT 0,
        state ENUM('idle', 'living', 'closed') DEFAULT 'idle',
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
        FOREIGN KEY (host_id) REFERENCES users(id)
      ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    `)
    
    await pool.execute(`
      CREATE TABLE IF NOT EXISTS danmaku_messages (
        id BIGINT AUTO_INCREMENT PRIMARY KEY,
        room_id INT NOT NULL,
        user_id INT NOT NULL,
        username VARCHAR(100) NOT NULL,
        content TEXT NOT NULL,
        color VARCHAR(20) DEFAULT '#00ff41',
        type ENUM('normal', 'gift', 'system') DEFAULT 'normal',
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        INDEX idx_room_created (room_id, created_at),
        FOREIGN KEY (room_id) REFERENCES rooms(id),
        FOREIGN KEY (user_id) REFERENCES users(id)
      ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    `)
    
    const [users] = await pool.execute('SELECT COUNT(*) as count FROM users WHERE id = 1')
    if (users[0].count === 0) {
      const [userResult] = await pool.execute(`
        INSERT INTO users (id, username, nickname, avatar, role)
        VALUES (1, 'ai_host', 'AI主播', 'https://api.dicebear.com/7.x/avataaars/svg?seed=host', 1)
      `)
      
      console.log(`👤 Host user created with ID: ${userResult.insertId}`)
      
      const [roomResult] = await pool.execute(`
        INSERT INTO rooms (id, room_name, host_id, host_name, title, cover_image, online_count, state)
        VALUES (1, 'AI直播间', 1, 'AI主播', '🔥 AI技术前沿直播 - 实时编程演示', 
                'https://api.dicebear.com/7.x/avataaars/svg?seed=host', 1234, 'living')
      `)
      
      console.log(`📺 Room created with ID: ${roomResult.insertId}`)
    }
    
    console.log('💾 Database initialized successfully')
    return true
  } catch (error) {
    console.error('❌ Database initialization failed:', error)
    throw error
  }
}

module.exports = {
  getMysqlPool,
  getRedisClient,
  initDatabase
}
