module.exports = {
  server: {
    port: process.env.API_PORT || 3000,
    host: '0.0.0.0'
  },
  
  database: {
    mysql: {
      host: process.env.MYSQL_HOST || '127.0.0.1',
      port: process.env.MYSQL_PORT || 3306,
      user: process.env.MYSQL_USER || 'chatroom',
      password: process.env.MYSQL_PASSWORD || 'chatroom123',
      database: process.env.MYSQL_DATABASE || 'chatroom_db',
      waitForConnections: true,
      connectionLimit: 10,
      queueLimit: 0
    },
    
    redis: {
      host: process.env.REDIS_HOST || '127.0.0.1',
      port: process.env.REDIS_PORT || 6379,
      password: process.env.REDIS_PASSWORD || '',
      db: 0
    }
  },
  
  cors: {
    origin: ['http://localhost:5173', 'http://localhost:5174'],
    credentials: true
  }
}
