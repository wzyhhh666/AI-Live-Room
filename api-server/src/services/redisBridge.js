const Redis = require('ioredis')

let instance = null

class RedisBridge {
  constructor(io) {
    this.io = io
    this.subscriber = new Redis({
      host: process.env.REDIS_HOST || '127.0.0.1',
      port: parseInt(process.env.REDIS_PORT || '6379'),
      lazyConnect: true
    })

    this.subscriber.on('error', (err) => {
      console.error('RedisBridge subscriber error:', err.message)
    })

    this.subscriber.on('connect', () => {
      console.log('RedisBridge subscriber connected')
    })
  }

  async start() {
    try {
      await this.subscriber.connect()

      await this.subscriber.subscribe('danmaku:output')
      await this.subscriber.subscribe('gift:output')
      await this.subscriber.subscribe('danmaku:blocked')
      await this.subscriber.subscribe('presence:online')

      this.subscriber.on('message', (channel, msg) => {
        try {
          const data = JSON.parse(msg)
          const roomId = data.room_id

          if (!roomId) {
            console.warn('RedisBridge: message without room_id on channel', channel)
            return
          }

          const roomName = `room-${roomId}`

          switch (channel) {
            case 'danmaku:output':
              this.io.to(roomName).emit('new-danmaku', data)
              break
            case 'gift:output':
              this.io.to(roomName).emit('new-gift', data)
              break
            case 'danmaku:blocked':
              this.io.to(roomName).emit('danmaku-blocked', data)
              break
            case 'presence:online':
              this.io.to(roomName).emit('online-count', data)
              break
            default:
              console.warn('RedisBridge: unknown channel', channel)
          }
        } catch (e) {
          console.error('RedisBridge parse error on channel', channel, ':', e.message)
        }
      })

      console.log('RedisBridge started, subscribed to 4 channels')
    } catch (err) {
      console.error('RedisBridge start failed:', err.message)
      throw err
    }
  }

  async stop() {
    try {
      if (this.subscriber) {
        await this.subscriber.quit()
      }
    } catch (err) {
      console.error('RedisBridge stop error:', err.message)
    }
  }
}

function getRedisBridge(io) {
  if (!io) {
    throw new Error('RedisBridge requires Socket.IO instance')
  }

  if (!instance) {
    instance = new RedisBridge(io)
  }
  return instance
}

module.exports = { getRedisBridge }
