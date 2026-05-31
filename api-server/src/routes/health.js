const { checkDatabase, checkRedis } = require('../middleware/healthCheck')

async function enhancedHealthCheck(req, res) {
  const db = require('../services/database')
  const pool = await db.getMysqlPool()
  const Redis = require('ioredis')

  const redisClient = new Redis({
    host: process.env.REDIS_HOST || '127.0.0.1',
    port: parseInt(process.env.REDIS_PORT || '6379'),
    lazyConnect: true,
    maxRetriesPerRequest: 1,
    retryStrategy: () => null
  })

  const [dbResult, redisResult] = await Promise.allSettled([
    checkDatabase(pool),
    checkRedis(redisClient)
  ])

  redisClient.disconnect()

  const checks = {
    database: dbResult.status === 'fulfilled' ? dbResult.value : { status: 'error', error: 'Check failed' },
    redis: redisResult.status === 'fulfilled' ? redisResult.value : { status: 'error', error: 'Check failed' }
  }

  const allOk = Object.values(checks).every(c => c.status === 'ok')
  const statusCode = allOk ? 200 : 503

  res.status(statusCode).json({
    status: allOk ? 'ok' : 'degraded',
    version: '1.0.0',
    timestamp: new Date().toISOString(),
    uptime_sec: Math.floor(process.uptime()),
    checks
  })
}

module.exports = { enhancedHealthCheck }
