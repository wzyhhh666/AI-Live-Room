# 模块 03 — 后端房间管理 开发规则

> **前置模块**: 模块 01（后端礼物系统）必须通过全部测试
> **前置阅读**: [全局主规则](../00-master-rules/MASTER_RULES.md)

---

## 1. 模块定位

扩展 `room.js` Service 和 `api.js` 路由，实现房间列表查询、创建/关闭房间、房间状态管理。

---

## 2. 文件变更清单

| 操作 | 文件路径 | 说明 |
|------|----------|------|
| **修改** | `api-server/src/services/room.js` | 追加 4 个方法 |
| **修改** | `api-server/src/routes/api.js` | 追加 4 个路由 |
| **修改** | `api-server/src/services/database.js` | 追加 room_members 表 DDL |
| **修改** | `docker/init-sql/01_init.sql` | 同步追加 |

---

## 3. 数据模型

### 3.1 room_members 表（新建 DDL）

```sql
CREATE TABLE IF NOT EXISTS room_members (
    id INT AUTO_INCREMENT PRIMARY KEY,
    room_id INT NOT NULL,
    user_id INT NOT NULL,
    username VARCHAR(100) NOT NULL,
    join_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    leave_time TIMESTAMP NULL DEFAULT NULL,
    UNIQUE KEY uk_room_user (room_id, user_id),
    FOREIGN KEY (room_id) REFERENCES rooms(id),
    FOREIGN KEY (user_id) REFERENCES users(id),
    INDEX idx_room_join (room_id, join_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
```

---

## 4. Service 新增方法

追加到现有 `room.js`（不改已有方法）：

```javascript
getRoomList(page, pageSize)            // 分页查询 living 状态房间，按 online_count 降序
createRoom(roomName, hostId, hostName, title, coverImage) // INSERT rooms，返回房间信息
closeRoom(roomId, hostId)              // 验证 host，UPDATE state='closed'
updateRoomMember(roomId, userId, username) // UPSERT room_members，进房时调用
```

### 详细签名

**`getRoomList(page = 1, pageSize = 20)`**
- `SELECT * FROM rooms WHERE state = 'living' ORDER BY online_count DESC LIMIT ? OFFSET ?`
- `SELECT COUNT(*) as total FROM rooms WHERE state = 'living'`
- 返回: `{ success: true, data: { rooms: [...], total, page, pageSize } }`
- rooms 数组中每个元素的字段映射与 `getRoomInfo` 一致：`roomId, roomName, hostId, hostName, title, coverImage, onlineCount, state`

**`createRoom(roomName, hostId, hostName, title, coverImage)`**
- INSERT rooms，`host_id`=hostId, `host_name`=hostName, `cover_image`=coverImage 或 NULL
- 返回生成房间的完整信息（与 `getRoomInfo` 格式一致）

**`closeRoom(roomId, hostId)`**
- 验证 `rooms.host_id === hostId`
- UPDATE `state = 'closed'`
- 返回: `{ success: true }` 或 `{ success: false, error: 'Not the host' }`

**`updateRoomMember(roomId, userId, username)`**
- `INSERT INTO room_members ... ON DUPLICATE KEY UPDATE join_time = VALUES(join_time), leave_time = NULL`
- 返回 `{ success: true }`

---

## 5. HTTP 路由

```javascript
GET    /api/rooms                  → roomService.getRoomList(page, pageSize)
POST   /api/rooms                  → roomService.createRoom(roomName, hostId, hostName, title, coverImage)
PUT    /api/rooms/:roomId/close    → roomService.closeRoom(roomId, hostId)
GET    /api/rooms/:roomId/members  → 查询 room_members WHERE leave_time IS NULL
```

**注意**: 新增路由与现有路由（`GET /api/rooms/:roomId`）不冲突。

---

## 6. Socket 集成

在 `socket.js` 的 `join-room` 事件处理器中，追加一行 `roomService.updateRoomMember(roomId, userId, username)`。

---

## 7. 关键约束

- ⚠️ **不修改 `getRoomInfo`、`sendDanmaku`、`updateOnlineCount` 的现有逻辑**
- ⚠️ `closeRoom` 必须验证 `host_id`，防止非主播关闭房间
- ⚠️ 房间列表只返回 `living` 状态的房间
- ⚠️ `createRoom` 返回的 `state` 默认为 `living`

---

*遵循以上规则和全局主规则进行开发。*
