# 模块 01 — 后端礼物系统 测试文档

> **测试执行条件**: 所有 Task 完成后，MySQL + Redis 已启动

---

## 测试环境准备

```bash
cd d:\ai直播项目\api-server
node src/server.js
```

确认控制台输出 `🚀 API Server is running!` 且无报错。

---

## 测试用例

### T1.1 — 礼物配置表创建

| 步骤 | 操作 | 预期结果 |
|------|------|----------|
| 1 | 启动服务器 | 日志显示 `💾 Database initialized successfully` |
| 2 | 连接 MySQL 查询 `SELECT * FROM gift_config` | 返回 6 行数据（荧光棒/点赞/鲜花/跑车/火箭/嘉年华） |

---

### T1.2 — HTTP 获取礼物列表

| 步骤 | 操作 | 预期结果 |
|------|------|----------|
| 1 | `curl http://localhost:3000/api/gifts` | 返回 `{"success":true,"data":[...]}` |
| 2 | 检查 data 数组 | 包含 6 个礼物，字段包括 `id,giftName,giftIcon,price,effectType` |
| 3 | 检查排序 | 按 `sortOrder` 升序排列 |

---

### T1.3 — HTTP 发送礼物

| 步骤 | 操作 | 预期结果 |
|------|------|----------|
| 1 | `curl -X POST http://localhost:3000/api/rooms/1/gift -H "Content-Type: application/json" -d '{"giftId":1,"count":3,"userId":2,"username":"TestUser"}'` | 返回 `{"success":true,"data":{...}}` |
| 2 | 检查 data.id | 为有效整数 |
| 3 | 检查 data.totalPrice | `3.00`（单价1元×3个） |
| 4 | 检查 data.effectType | `"normal"` |
| 5 | 查询 MySQL `SELECT * FROM gift_record ORDER BY created_at DESC LIMIT 1` | 存在对应记录，total_price=3.00 |

---

### T1.4 — HTTP 获取排行榜

| 步骤 | 操作 | 预期结果 |
|------|------|----------|
| 1 | 执行 T1.3 多次（不同 user） | 写入多条礼物记录 |
| 2 | `curl http://localhost:3000/api/rooms/1/gift/rank` | 返回 `{"success":true,"data":[...]}` |
| 3 | 检查 data 数组 | 按 totalAmount 降序排列 |
| 4 | 检查格式 | 每项包含 `senderId, senderName, totalAmount` |

---

### T1.5 — 送礼参数校验

| 步骤 | 操作 | 预期结果 |
|------|------|----------|
| 1 | POST 送礼缺少 giftId | 返回 `{"success":false,"error":"Missing required fields"}` |
| 2 | POST 送礼 giftId=999（不存在） | 返回 `{"success":false,"error":"Gift not found"}` |
| 3 | POST 送礼 count=0 | 返回 `{"success":false,"error":"Invalid count"}` |

---

### T1.6 — 服务器无报错启动

| 步骤 | 操作 | 预期结果 |
|------|------|----------|
| 1 | `node src/server.js` | 启动成功，无 `❌` 错误日志 |
| 2 | 检查所有 require  | 无 `MODULE_NOT_FOUND` 错误 |

---

## 通过标准

全部 6 个测试用例通过，服务器正常启动且 HTTP 返回正确，方可进入下一个模块开发。
