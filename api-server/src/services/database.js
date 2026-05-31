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
        like_count INT DEFAULT 0,
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

    await pool.execute(`
      CREATE TABLE IF NOT EXISTS gift_record (
        record_id   BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
        room_id     INT NOT NULL,
        sender_id   INT NOT NULL,
        sender_name VARCHAR(100) NOT NULL DEFAULT '',
        receiver_id INT NOT NULL,
        gift_id     INT UNSIGNED NOT NULL,
        gift_name   VARCHAR(100) NOT NULL DEFAULT '',
        gift_count  SMALLINT UNSIGNED NOT NULL DEFAULT 1,
        total_price DECIMAL(10,2) NOT NULL DEFAULT 0.00,
        effect_type VARCHAR(50) NOT NULL DEFAULT 'normal',
        created_at  TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        PRIMARY KEY (record_id),
        INDEX idx_room_time (room_id, created_at)
      ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    `)

    await pool.execute(`
      CREATE TABLE IF NOT EXISTS gift_config (
        id INT AUTO_INCREMENT PRIMARY KEY,
        gift_name VARCHAR(50) NOT NULL,
        gift_icon VARCHAR(10) NOT NULL COMMENT 'emoji图标',
        price DECIMAL(10,2) NOT NULL DEFAULT 0.00,
        effect_type VARCHAR(20) DEFAULT 'normal' COMMENT '特效类型: normal/explosion/rain/rocket',
        sort_order INT DEFAULT 0,
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
      ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    `)
    
    await pool.execute(`
      CREATE TABLE IF NOT EXISTS room_members (
        id INT AUTO_INCREMENT PRIMARY KEY,
        room_id INT NOT NULL,
        user_id INT NOT NULL,
        username VARCHAR(100) NOT NULL,
        join_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        leave_time TIMESTAMP NULL DEFAULT NULL,
        UNIQUE KEY uk_room_user (room_id, user_id),
        FOREIGN KEY (room_id) REFERENCES rooms(id),
        FOREIGN KEY (user_id) REFERENCES users(id),
        INDEX idx_room_join (room_id, join_time)
      ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    `)

    await pool.execute(`
      CREATE TABLE IF NOT EXISTS stream_records (
        id BIGINT AUTO_INCREMENT PRIMARY KEY,
        room_id INT NOT NULL,
        stream_key VARCHAR(255) NOT NULL COMMENT '推流密钥',
        protocol ENUM('rtmp', 'srt', 'webrtc') DEFAULT 'rtmp',
        resolution VARCHAR(20) COMMENT '分辨率 1920x1080',
        bitrate INT COMMENT '码率 kbps',
        fps INT COMMENT '帧率',
        started_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        ended_at TIMESTAMP NULL,
        duration_sec INT DEFAULT 0,
        dvr_path VARCHAR(500) COMMENT '录制文件路径',
        INDEX idx_room_time (room_id, started_at),
        FOREIGN KEY (room_id) REFERENCES rooms(id)
      ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
    `)

    await pool.execute(`
      CREATE TABLE IF NOT EXISTS sensitive_log (
        id BIGINT AUTO_INCREMENT PRIMARY KEY,
        room_id INT NOT NULL,
        user_id INT NULL,
        original_content TEXT NOT NULL,
        filtered_content TEXT,
        matched_words VARCHAR(500),
        action ENUM('mask', 'block') NOT NULL,
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        INDEX idx_created (created_at),
        FOREIGN KEY (room_id) REFERENCES rooms(id) ON DELETE CASCADE,
        FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE SET NULL
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

      await pool.execute(`
        INSERT IGNORE INTO gift_config (id, gift_name, gift_icon, price, effect_type, sort_order) VALUES
        (1, '荧光棒', '🪄', 1.00, 'normal', 1),
        (2, '点赞', '👍', 2.00, 'normal', 2),
        (3, '鲜花', '🌸', 5.00, 'rain', 3),
        (4, '跑车', '🏎️', 50.00, 'rocket', 4),
        (5, '火箭', '🚀', 100.00, 'explosion', 5),
        (6, '嘉年华', '🎪', 500.00, 'explosion', 6)
      `)
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
