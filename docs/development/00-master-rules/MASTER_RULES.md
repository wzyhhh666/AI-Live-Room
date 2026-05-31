# AI Live Room — 全局开发主规则

> 本文档是所有模块开发的**最高优先规则**，每个模块开发前必须先阅读本文档。

---

## 1. 项目集成约束

### 1.1 后端（api-server）强制规则

| 规则编号 | 规则内容 |
|----------|----------|
| **B-01** | 所有新 Service 放在 `api-server/src/services/` 目录，与现有 `user.js`、`room.js`、`database.js`、`socket.js` 同级 |
| **B-02** | 所有新 Route 统一追加到 `api-server/src/routes/api.js`，不新建路由文件 |
| **B-03** | 数据库访问**必须**通过 `database.js` 的 `getMysqlPool()` 和 `getRedisClient()` 获取连接，**禁止**直接创建连接 |
| **B-04** | Service 导出格式统一为 `module.exports = { fn1, fn2, ... }`，与现有 `room.js`、`user.js` 一致 |
| **B-05** | 返回值格式统一为 `{ success: boolean, data?: any, error?: string }` |
| **B-06** | 错误处理统一 try/catch + `console.error()`，**不抛未捕获异常** |
| **B-07** | WebSocket 事件处理器注册在 `socket.js` 的 `io.on('connection')` 回调内 |
| **B-08** | **广播规则（已修Bug#1）**: 对发送者自身也要收到的消息用 `io.to(room)`，发送者不需要重复收到的用 `socket.broadcast.to(room)` |
| **B-09** | 新模块与现有模块通过 `require('../services/xxx')` 引用，不循环依赖 |

### 1.2 数据库约束

| 规则编号 | 规则内容 |
|----------|----------|
| **DB-01** | 表名和字段名与 `database.js` 的 `initDatabase()` 创建的完全一致：`users`、`rooms`、`danmaku_messages`、`gift_record` |
| **DB-02** | 新增 SQL init 语句同步追加到 `docker/init-sql/01_init.sql` |
| **DB-03** | 所有查询使用 `pool.execute(sql, params)` 参数化查询防注入 |
| **DB-04** | `role` 字段：0=观众, 1=主播, 2=管理员 |
| **DB-05** | `rooms.state` 枚举：`idle`, `living`, `closed` — 与现有代码一致 |

### 1.3 前端（frontend）强制规则

| 规则编号 | 规则内容 |
|----------|----------|
| **F-01** | 所有新组件放在 `frontend/src/components/` 对应子目录下 |
| **F-02** | 所有新 Store 放在 `frontend/src/stores/` 下，使用 Pinia `defineStore` + Composition API 风格 |
| **F-03** | **状态管理原则**: 全局共享状态通过 Store 管理，组件本地状态用 `ref()`，**禁止组件维护与 Store 重复的数据**（已修Bug#5） |
| **F-04** | API 调用通过 `frontend/src/services/api.ts` 的 `apiService` 实例，新方法追加到 `ApiService` 类中 |
| **F-05** | UI 风格延续赛博朋克科技风：背景 `#0F0F23`，主色渐变色系，毛玻璃效果（`backdrop-blur`），Tailwind CSS 原子类 |
| **F-06** | TypeScript 严格模式，所有 props/emit/store 必须有类型定义 |
| **F-07** | 字体：标题 `Orbitron`（Google Fonts），正文系统默认 |
| **F-08** | 图标使用 `lucide-vue-next`，不引入额外图标库 |
| **F-09** | Vue 文件结构：`<script setup lang="ts">` → `<template>` → `<style scoped>` |
| **F-10** | 路由中新增页面的 meta 必须设置 `requiresAuth: true` |

### 1.4 通信协议约束

| 规则编号 | 规则内容 |
|----------|----------|
| **C-01** | Socket 事件名全部小写，单词用 `-` 连接：`send-gift`、`new-gift`、`gift-rank` |
| **C-02** | 客户端→服务器事件命名前缀动词：`send-`/`join-`/`leave-`/`get-` |
| **C-03** | 服务器→客户端事件命名前缀形容词/名词：`new-`/`recent-`/`online-` |
| **C-04** | Socket 消息 payload 使用 JSON 对象，不嵌套过深（≤3层） |
| **C-05** | HTTP API 路由遵循 RESTful：`GET /api/resource`（列表）、`GET /api/resource/:id`（详情）、`POST /api/resource`（创建）、`PUT /api/resource/:id`（更新）、`DELETE /api/resource/:id`（删除） |

### 1.5 代码风格约束

| 规则编号 | 规则内容 |
|----------|----------|
| **S-01** | 后端 JavaScript 使用 `const`/`let`，不写分号（遵循现有文件风格） |
| **S-02** | 前端 TypeScript 使用 `const`/`let`，写分号 |
| **S-03** | 缩进 2 空格 |
| **S-04** | 不添加注释（除非用户明确要求）—— 代码自文档化 |

---

## 2. 现有代码集成点速查

### 2.1 后端数据库表现状

```sql
-- 以下是 database.js initDatabase() 当前创建的表
users:    id, username, nickname, avatar, role, token, created_at, updated_at
rooms:    id, room_name, host_id, host_name, title, cover_image, online_count, state
danmaku_messages: id, room_id, user_id, username, content, color, type, created_at

-- gift_record 表当前仅在 Docker init SQL 中定义，模块 01 开发时须同步追加到 database.js
-- gift_record: record_id, room_id, sender_id, receiver_id, gift_id, gift_count, total_price, created_at
```

### 2.2 前端 Store 现状

| Store | 文件 | 关键状态/方法 |
|-------|------|--------------|
| `useUserStore` | `stores/user.ts` | `userInfo`, `isLoggedIn`, `displayName`, `login()`, `logout()` |
| `useRoomStore` | `stores/room.ts` | `currentRoom`, `danmakuList`, `recentDanmaku`, `addDanmaku()`, `joinRoom()`, `leaveRoom()` |

### 2.3 WebSocket 事件现状

| 方向 | 事件名 | 说明 |
|------|--------|------|
| C→S | `join-room` | 加入房间 `{roomId, userInfo}` |
| C→S | `send-danmaku` | 发送弹幕 `{roomId, content}` |
| C→S | `typing` | 输入状态 |
| S→C | `joined-room` | 加入确认 |
| S→C | `room-info` | 房间信息 |
| S→C | `recent-danmaku` | 历史弹幕 |
| S→C | `new-danmaku` | 新弹幕（仅广播给其他人） |
| S→C | `online-count` | 在线人数 |
| S→C | `stats-update` | 统计数据 |

---

## 3. 开发流程规则

1. **每模块开发前**: 阅读本节 + 该模块的 RULES.md
2. **开发中**: 严格按照 TASKS.md 步骤顺序执行
3. **完成后**: 执行 TESTS.md 中的所有测试项，全部通过才进入下一模块
4. **验证命令**:
   - 后端: `cd api-server && node src/server.js`（检查启动日志无报错）
   - 前端: `cd frontend && npx vue-tsc --noEmit && npx vite build`
5. **不得跨模块开发**: 一个模块通过全部测试后，再开始下一个模块

---

## 4. 错误修复记录（避免回归）

| Bug# | 问题 | 修复 | 日期 |
|------|------|------|------|
| #1 | 发送弹幕双份显示 | `socket.js`: `io.to()` → `socket.broadcast.to()` | 2026-05-19 |
| #2 | 发弹幕错误增加online_count | `room.js`: 删除 `UPDATE rooms SET online_count = online_count + 1` | 2026-05-19 |
| #3 | DB Schema 不一致 | `01_init.sql` 重写为与 `database.js` 一致的 schema | 2026-05-19 |
| #4 | 字体 typo `display=translate` | RoomView.vue 改为 `display=swap` | 2026-05-19 |
| #5 | Store 被架空 | RoomView 本地 danmakuList 迁移到 roomStore | 2026-05-19 |
| #6 | top-level await 构建失败 | `main.ts` 改为同步 import | 2026-05-19 |

---

*本文档优先级最高，所有模块开发必须遵守上述规则。*
