async function checkDatabase(dbPool) {
  const start = Date.now()
  try {
    await dbPool.execute('SELECT 1')
    return { status: 'ok', latency_ms: Date.now() - start }
  } catch {
    return { status: 'error', latency_ms: Date.now() - start, error: 'Database unreachable' }
  }
}

async function checkRedis(redisClient) {
  const start = Date.now()
  try {
    await redisClient.ping()
    return { status: 'ok', latency_ms: Date.now() - start }
  } catch {
    return { status: 'error', latency_ms: Date.now() - start, error: 'Redis unreachable' }
  }
}

module.exports = { checkDatabase, checkRedis }
