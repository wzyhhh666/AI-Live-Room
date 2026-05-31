# 模块 03 — 后端房间管理 测试文档

> **前置条件**: 模块 01 全部测试通过

---

## 测试用例

### T3.1 — room_members 表创建

| 步骤 | 操作 | 预期结果 |
|------|------|----------|
| 1 | 启动服务器 | 日志无 SQL 错误 |
| 2 | `SHOW TABLES LIKE 'room_members'` | 存在该表 |

---

### T3.2 — 房间列表查询

| 步骤 | 操作 | 预期结果 |
|------|------|----------|
| 1 | `curl http://localhost:3000/api/rooms` | 返回 `{"success":true,"data":{"rooms":[...],"total":N}}` |
| 2 | 检查 rooms 数组 | 每个包含 roomId/roomName/hostName/onlineCount/state/coverImage |
| 3 | 检查排序 | 按 onlineCount 降序 |

---

### T3.3 — 创建房间

| 步骤 | 操作 | 预期结果 |
|------|------|----------|
| 1 | `curl -X POST http://localhost:3000/api/rooms -H "Content-Type: application/json" -d '{"roomName":"TestRoom","hostId":1,"hostName":"TestHost","title":"测试直播间","coverImage":"https://via.placeholder.com/400x225"}'` | 返回 `{"success":true,"data":{...}}` |
| 2 | 检查 data.state | `"living"` |
| 3 | 检查 data.roomName | `"TestRoom"` |

---

### T3.4 — 关闭房间（验证权限）

| 步骤 | 操作 | 预期结果 |
|------|------|----------|
| 1 | `curl -X PUT http://localhost:3000/api/rooms/{新房间ID}/close -H "Content-Type: application/json" -d '{"hostId":999}'` | `{"success":false,"error":"Not the host"}` |
| 2 | `curl -X PUT http://localhost:3000/api/rooms/{新房间ID}/close -H "Content-Type: application/json" -d '{"hostId":1}'` | `{"success":true}` |
| 3 | `curl http://localhost:3000/api/rooms/{新房间ID}` | data.state = `"closed"` |

---

### T3.5 — 进入房间记录成员

| 步骤 | 操作 | 预期结果 |
|------|------|----------|
| 1 | 通过 WebSocket join-room | 正常加入 |
| 2 | 查询 room_members | 存在该用户记录，leave_time IS NULL |
| 3 | 同一用户再次 join-room | 只更新 join_time，不创建重复行 |

---

### T3.6 — 无报错启动

| 步骤 | 操作 | 预期结果 |
|------|------|----------|
| 1 | `node src/server.js` | 无 `❌` 日志，启动成功 |

---

## 通过标准

全部 6 个测试用例通过。
