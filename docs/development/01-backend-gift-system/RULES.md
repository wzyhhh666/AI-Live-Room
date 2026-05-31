# 模块 01 — 后端礼物系统 开发规则

> **前置阅读**: [全局主规则](../00-master-rules/MASTER_RULES.md)
> **配套文档**: [TASKS.md](./TASKS.md) | [TESTS.md](./TESTS.md)

---

## 1. 模块定位

在 `api-server` 中新建 `gift.js` Service，并在 `api.js` 路由和 `socket.js` 中追加礼物相关逻辑，实现完整的礼物收发后端能力。

---

## 2. 文件变更清单

| 操作 | 文件路径 | 说明 |
|------|----------|------|
| **新建** | `api-server/src/services/gift.js` | 礼物业务逻辑（核心） |
| **修改** | `api-server/src/routes/api.js` | 追加 3 个礼物 HTTP 路由 |
| **修改** | `api-server/src/services/socket.js` | 追加 `send-gift` WebSocket 事件 |
| **修改** | `api-server/src/services/database.js` | 追加礼物配置表 DDL |
| **修改** | `docker/init-sql/01_init.sql` | 同步追加 DDL |

---

## 3. 数据模型

### 3.1 gift_config 表（新建，DDL 追加到 database.js initDatabase()）

```sql
CREATE TABLE IF NOT EXISTS gift_config (
    id INT AUTO_INCREMENT PRIMARY KEY,
    gift_name VARCHAR(50) NOT NULL,
    gift_icon VARCHAR(10) NOT NULL COMMENT 'emoji图标',
    price DECIMAL(10,2) NOT NULL DEFAULT 0.00,
    effect_type VARCHAR(20) DEFAULT 'normal' COMMENT '特效类型: normal/explosion/rain/rocket',
    sort_order INT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
```

### 3.2 gift_record 表（已存在于 Docker init SQL，开发时须同步 DDL 到 database.js）

```sql
gift_record: record_id, room_id, sender_id, receiver_id, gift_id, gift_count, total_price, created_at
```

### 3.3 初始数据（追加到 database.js 的种子数据部分）

```sql
INSERT IGNORE INTO gift_config (id, gift_name, gift_icon, price, effect_type, sort_order) VALUES
(1, '荧光棒', '🪄', 1.00, 'normal', 1),
(2, '点赞', '👍', 2.00, 'normal', 2),
(3, '鲜花', '🌸', 5.00, 'rain', 3),
(4, '跑车', '🏎️', 50.00, 'rocket', 4),
(5, '火箭', '🚀', 100.00, 'explosion', 5),
(6, '嘉年华', '🎪', 500.00, 'explosion', 6)
```

---

## 4. Service 接口规范

### 4.1 `gift.js` 导出方法

```javascript
module.exports = {
  getGiftList,      // () → {success, data: GiftConfig[]}
  sendGift,         // (roomId, giftId, count, senderId, senderName) → {success, data: GiftRecord}
  getGiftRank,      // (roomId, limit) → {success, data: [{senderId, senderName, totalAmount}]}
}
```

### 4.2 方法详细签名

**`getGiftList()`**
- 从 `gift_config` 表查询所有礼物，按 `sort_order` 排序
- 返回: `{ success: true, data: [{id, giftName, giftIcon, price, effectType, sortOrder}] }`

**`sendGift(roomId, giftId, count, senderId, senderName)`**
- 从 `gift_config` 查询礼物价格
- 计算 `totalPrice = price * count`
- INSERT 到 `gift_record` 表（receiver_id = 房间 host_id）
- Redis `ZINCRBY rank:room:{roomId}:gift {totalPrice} {senderId}:{senderName}`
- 返回: `{ success: true, data: { id, giftName, giftIcon, count, totalPrice, senderName, effectType, roomId, timestamp } }`

**`getGiftRank(roomId, limit = 20)`**
- Redis `ZREVRANGE rank:room:{roomId}:gift 0 {limit-1} WITHSCORES`
- 返回: `{ success: true, data: [{senderId, senderName, totalAmount}] }`

---

## 5. HTTP API 路由

追加到 `api.js`，不得冲突现有路由：

```javascript
GET  /api/gifts                        → giftService.getGiftList()
POST /api/rooms/:roomId/gift           → giftService.sendGift(roomId, req.body.giftId, req.body.count, req.body.userId, req.body.username)
GET  /api/rooms/:roomId/gift/rank      → giftService.getGiftRank(roomId, req.query.limit)
```

---

## 6. WebSocket 事件

在 `socket.js` 的 `connection` 回调中追加：

```javascript
socket.on('send-gift', async ({ roomId, giftId, count }) => {
  // 1. 校验 userInfo 存在（与 send-danmaku 同样校验）
  // 2. 调用 giftService.sendGift()
  // 3. 用 socket.broadcast.to(room) 广播 new-gift（发送者前端乐观渲染）
  // 4. 用 io.to(room) 广播 gift-rank-update（排行榜大家都要更新）
})
```

---

## 7. 集成规则

| 规则 | 内容 |
|------|------|
| **不要修改现有代码逻辑** | 只追加代码，不删除/修改现有 `room.js`、`user.js`、`socket.js` 中已有的事件处理 |
| **复用 database.js** | 通过 `const db = require('./database')` 获取连接 |
| **与 socket.js 的交互** | `gift.js` 不依赖 socket.js，由 socket.js 调用 gift.js 后自行广播 |
| **数据库种子数据** | 追加到 `database.js` 的 `initDatabase()` 中，在 users 种子数据之后 |

---

## 8. 关键约束

- ⚠️ **不要硬编码主播ID=1**：从 `rooms` 表查询 `host_id` 作为 `receiver_id`
- ⚠️ **总价计算**: `totalPrice = price * count`，金额以元为单位，存 DECIMAL(10,2)
- ⚠️ **Redis Key**: `rank:room:{roomId}:gift` 使用 ZSET，score 累计总金额
- ⚠️ **Redis 不设置 TTL**: 排行榜 Key 永久保留

---

*遵循以上规则和全局主规则进行开发。*
