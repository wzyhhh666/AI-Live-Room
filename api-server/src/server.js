const express = require('express')
const http = require('http')
const { Server } = require('socket.io')
const cors = require('cors')
const config = require('../config/database')
const db = require('./services/database')

const app = express()
const server = http.createServer(app)
const io = new Server(server, {
  cors: {
    origin: config.cors.origin,
    methods: ['GET', 'POST'],
    credentials: true
  }
})

app.use(cors(config.cors))
app.use(express.json())

const apiRoutes = require('./routes/api')
app.use('/api', apiRoutes)

require('./services/socket')(io)

async function startServer() {
  try {
    console.log('🔧 Initializing database...')
    await db.initDatabase()
    
    const PORT = config.server.port
    server.listen(PORT, config.server.host, () => {
      console.log('====================================')
      console.log('🚀 API Server is running!')
      console.log(`📍 HTTP Server: http://${config.server.host}:${PORT}`)
      console.log(`🔌 WebSocket Server: ws://${config.server.host}:${PORT}`)
      console.log(`📊 Environment: ${process.env.NODE_ENV || 'development'}`)
      console.log('====================================')
      console.log('\n📡 Available Endpoints:')
      console.log('  GET  /api/health          - Health check')
      console.log('  POST /api/auth/login       - User login')
      console.log('  GET  /api/rooms/:roomId    - Get room info')
      console.log('  GET  /api/rooms/:roomId/danmaku - Get recent danmaku')
      console.log('  POST /api/rooms/:roomId/danmaku - Send danmaku')
      console.log('\n🔌 WebSocket Events:')
      console.log('  join-room     - Join a room')
      console.log('  send-danmaku  - Send danmaku message')
      console.log('  typing        - User typing indicator')
      console.log('====================================\n')
    })
    
  } catch (error) {
    console.error('❌ Failed to start server:', error)
    process.exit(1)
  }
}

startServer()

module.exports = { app, io, server }
