# 模块 05 — 后端弹幕限流器 开发任务

> **前置条件**: 模块 01 测试通过

---

## Task 5.1: 创建 rateLimiter.js

**文件**: `api-server/src/services/rateLimiter.js`（新建）

实现 `checkDanmakuLimit(userId, roomId)` 方法，逻辑见 RULES.md

---

## Task 5.2: 集成到 socket.js

**文件**: `api-server/src/services/socket.js`

在 `send-danmaku` 处理器中，`content` 获取后、`roomService.sendDanmaku` 调用前，加入限流检查

---

## Task 5.3: 启动验证

```bash
node src/server.js
```

手动测试：在 1 秒内发送 2 条弹幕，第 2 条应收到 `{ message: 'Too fast' }` 的 error 事件

---

*完成后运行 TESTS.md*
