const db = require('./database')

async function checkDanmakuLimit(userId, roomId) {
  const redis = await db.getRedisClient()

  const userKey = `limit:danmaku:user:${userId}`
  const userCount = await redis.incr(userKey)
  if (userCount === 1) await redis.expire(userKey, 1)
  if (userCount > 1) return { allowed: false, reason: 'Too fast' }

  const roomKey = `limit:danmaku:room:${roomId}`
  const roomCount = await redis.incr(roomKey)
  if (roomCount === 1) await redis.expire(roomKey, 1)
  if (roomCount > 100) return { allowed: false, reason: 'Room too busy' }

  return { allowed: true }
}

module.exports = { checkDanmakuLimit }
