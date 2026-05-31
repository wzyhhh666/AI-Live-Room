# 模块 03 — 后端房间管理 开发任务

> **前置条件**: 模块 01 全部测试通过
> **前置阅读**: [RULES.md](./RULES.md)

---

## 任务执行顺序

---

### Task 3.1: 追加 room_members 表 DDL

**文件**: `api-server/src/services/database.js`

在 `gift_config` 建表语句之后，追加 `room_members` 表的 `CREATE TABLE IF NOT EXISTS`

**同步**: `docker/init-sql/01_init.sql`

---

### Task 3.2: 追加 room.js 方法

**文件**: `api-server/src/services/room.js`

在 `module.exports` 之上，文件末尾追加 4 个新方法：
1. `getRoomList(page, pageSize)`
2. `createRoom(roomName, hostId, hostName, title)`
3. `closeRoom(roomId, hostId)`
4. `updateRoomMember(roomId, userId, username)`

**重要**: 不修改现有的 `getRoomInfo`、`sendDanmaku`、`updateOnlineCount`、`getRecentDanmaku`

---

### Task 3.3: 追加 HTTP 路由

**文件**: `api-server/src/routes/api.js`

在 `module.exports = router` 之前追加 4 个新路由（放在礼物路由之后）：
1. `GET /api/rooms` — 房间列表
2. `POST /api/rooms` — 创建房间
3. `PUT /api/rooms/:roomId/close` — 关闭房间
4. `GET /api/rooms/:roomId/members` — 房间成员

**注意**: 与现有 `GET /api/rooms/:roomId` 不冲突（Express 路由匹配优先静态路径）

---

### Task 3.4: Socket join-room 添加成员记录

**文件**: `api-server/src/services/socket.js`

在 `join-room` 事件处理中，`socket.data.roomId = roomId` 之后，追加：
```javascript
await roomService.updateRoomMember(roomId, userInfo.userId, userInfo.username)
```

---

*所有任务完成后，运行 TESTS.md 中的全部测试用例。*
