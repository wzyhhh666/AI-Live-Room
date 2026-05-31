# 模块 01 — 后端礼物系统 开发任务

> **前置阅读**: [RULES.md](./RULES.md) | [全局主规则](../00-master-rules/MASTER_RULES.md)

---

## 任务执行顺序（严格按序号执行）

---

### Task 1.1: 追加 gift_config 表到数据库初始化

**文件**: `api-server/src/services/database.js`

**操作**:
1. 在 `initDatabase()` 函数中，`danmaku_messages` 建表语句之后，追加 `gift_record` 表的 DDL（从 `01_init.sql` 复制）和 `gift_config` 表的 `CREATE TABLE IF NOT EXISTS` 语句
2. 在 `INSERT IGNORE INTO rooms` 种子数据之后，追加 `gift_config` 的种子数据 `INSERT IGNORE`

**验证**: 启动 `node src/server.js`，检查日志中 MySQL 无报错

---

### Task 1.2: 创建 gift.js Service 文件

**文件**: `api-server/src/services/gift.js`（新建）

**实现内容**:
1. `const db = require('./database')`
2. `getGiftList()` — 查询 `gift_config` 表，返回字段映射为驼峰
3. `sendGift(roomId, giftId, count, senderId, senderName)` — 完整送礼逻辑
4. `getGiftRank(roomId, limit)` — 从 Redis ZSET 查询排行榜

**关键要点**:
- 送礼时需查询 `gift_config` 获取价格
- 送礼时需查询 `rooms` 获取 `host_id` 作为 `receiver_id`
- 使用 `ZINCRBY` 累加排行榜分数

---

### Task 1.3: 追加礼物 HTTP 路由

**文件**: `api-server/src/routes/api.js`

**操作**:
1. 引入 `const giftService = require('../services/gift')`
2. 在 `module.exports = router` 之前追加 3 个路由：
   - `GET /gifts` — 礼物列表
   - `POST /rooms/:roomId/gift` — 发送礼物
   - `GET /rooms/:roomId/gift/rank` — 礼物排行榜

---

### Task 1.4: 追加 send-gift WebSocket 事件

**文件**: `api-server/src/services/socket.js`

**操作**:
1. 引入 `const giftService = require('./gift')`
2. 在 `send-danmaku` 事件处理器之后，追加 `send-gift` 事件处理器
3. 广播逻辑：
   - `socket.broadcast.to(room).emit('new-gift', data)` — 其他人收到礼物通知
   - `io.to(room).emit('gift-rank-update', rankData)` — 榜单全房间更新

---

### Task 1.5: 同步更新 Docker init SQL

**文件**: `docker/init-sql/01_init.sql`

**操作**: 将 `gift_config` 的 CREATE TABLE 和 INSERT IGNORE 语句追加到文件末尾

---

*所有任务完成后，运行 TESTS.md 中的全部测试用例。*
