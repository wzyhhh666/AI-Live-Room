# 模块 05 — 后端弹幕限流器 开发规则

> **前置模块**: 模块 01 必须通过全部测试
> **前置阅读**: [全局主规则](../00-master-rules/MASTER_RULES.md)

---

## 1. 模块定位

在 `socket.js` 的 `send-danmaku` 事件处理器中追加 Redis 滑动窗口限流，防止用户刷屏。

---

## 2. 文件变更清单

| 操作 | 文件路径 | 说明 |
|------|----------|------|
| **新建** | `api-server/src/services/rateLimiter.js` | 限流器工具 |
| **修改** | `api-server/src/services/socket.js` | 在 send-danmaku 中集成限流器 |

---

## 3. rateLimiter.js 接口

```javascript
const db = require('./database')

async function checkDanmakuLimit(userId, roomId) {
  const redis = await db.getRedisClient()
  
  // 用户级：每秒最多 1 条
  const userKey = `limit:danmaku:user:${userId}`
  const userCount = await redis.incr(userKey)
  if (userCount === 1) await redis.expire(userKey, 1)
  if (userCount > 1) return { allowed: false, reason: 'Too fast' }
  
  // 房间级：每秒最多 100 条
  const roomKey = `limit:danmaku:room:${roomId}`
  const roomCount = await redis.incr(roomKey)
  if (roomCount === 1) await redis.expire(roomKey, 1)
  if (roomCount > 100) return { allowed: false, reason: 'Room too busy' }
  
  return { allowed: true }
}

module.exports = { checkDanmakuLimit }
```

---

## 4. Socket 集成

在 `socket.js` 的 `send-danmaku` 处理器中，`content` 获取之后、`roomService.sendDanmaku` 之前：

```javascript
const limitResult = await rateLimiter.checkDanmakuLimit(userId, roomId)
if (!limitResult.allowed) {
  socket.emit('error', { message: limitResult.reason })
  return
}
```

---

## 5. 关键约束

- ⚠️ 只修改 `socket.js` 中 `send-danmaku` 处理器的开头部分
- ⚠️ 用户级 1条/秒、房间级 100条/秒
- ⚠️ 超频时 return（不写入数据库、不广播）
- ⚠️ 前端已有错误监听（`socket.on('error', ...)`），无需改前端

---

*遵循以上规则和全局主规则进行开发。*
