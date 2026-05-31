const express = require('express')
const http = require('http')
const { Server } = require('socket.io')
const cors = require('cors')
const path = require('path')
require('dotenv').config({ path: path.join(__dirname, '..', '.env') })
const config = require('../config/database')
const db = require('./services/database')
const sensitiveFilter = require('./services/sensitiveFilter')
const { getRedisBridge } = require('./services/redisBridge')
const { errorHandler, notFoundHandler } = require('./middleware/errorHandler')

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

const streamRoutes = require('./routes/stream')
app.use('/api/stream', streamRoutes)

app.set('io', io)

require('./services/socket')(io)

app.use(notFoundHandler)
app.use(errorHandler)

async function startServer() {
  try {
    console.log('🔧 Initializing database...')
    await db.initDatabase()
    
    const pool = await db.getMysqlPool()
    const [staleResult] = await pool.execute(
      "UPDATE rooms SET state = 'idle' WHERE state = 'living'"
    )
    if (staleResult.changedRows > 0) {
      console.log(`🧹 Cleaned up ${staleResult.changedRows} stale room(s) from previous session`)
    }
    
    pool.execute(
      "UPDATE stream_records SET ended_at = NOW(), duration_sec = TIMESTAMPDIFF(SECOND, started_at, NOW()) WHERE ended_at IS NULL"
    )
    
    sensitiveFilter.init()
    
    const redisBridge = getRedisBridge(io)
    redisBridge.start().catch(err => {
      console.error('Failed to start RedisBridge:', err.message)
    })
    
    const PORT = config.server.port
    server.listen(PORT, config.server.host, () => {
      console.log('====================================')
      console.log('🚀 API Server is running!')
      console.log(`📍 HTTP Server: http://${config.server.host}:${PORT}`)
      console.log(`🔌 WebSocket Server: ws://${config.server.host}:${PORT}`)
      console.log(`📊 Environment: ${process.env.NODE_ENV || 'development'}`)
      console.log('====================================')
      console.log('\n📡 Available Endpoints:')
      console.log('  GET  /api/health               - Health check')
      console.log('  POST /api/auth/login            - User login')
      console.log('  GET  /api/rooms                 - Room list')
      console.log('  POST /api/rooms                 - Create room')
      console.log('  GET  /api/rooms/:roomId         - Get room info')
      console.log('  PUT  /api/rooms/:roomId/close   - Close room')
      console.log('  GET  /api/rooms/:roomId/members - Get room members')
      console.log('  GET  /api/rooms/:roomId/danmaku - Get recent danmaku')
      console.log('  POST /api/rooms/:roomId/danmaku - Send danmaku')
      console.log('  GET  /api/gifts                 - Gift list')
      console.log('  POST /api/rooms/:roomId/gift    - Send gift')
      console.log('  GET  /api/rooms/:roomId/gift/rank - Gift rank')
      console.log('  POST /api/stream/callback        - SRS publish/unpublish callback')
      console.log('  GET  /api/stream/rooms/:roomId/stream/info - Stream info')
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
