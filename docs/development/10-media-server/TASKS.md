# 模块 10 — 流媒体服务器部署与集成 开发任务

> **前置阅读**: [RULES.md](./RULES.md) | [全局主规则](../00-master-rules/MASTER_RULES.md) | [架构设计](../../ARCHITECTURE_DESIGN.md#41-流媒体服务器层-srs)

---

## 任务执行顺序（严格按序号执行）

---

### Task 10.1: 创建 SRS 配置文件

**文件**: `srs/srs.conf`（项目根目录下新建）

**操作**:
1. 在项目根目录新建 `srs/` 目录
2. 创建 `srs.conf`，内容参考 RULES.md 第 4.2 节配置模板
3. 确认 `http_hooks` URL 使用 `host.docker.internal:3000`（Windows Docker）
4. 确认 `crossdomain on;` 拼写正确（无下划线）

**验证**: 文件存在且语法完整

---

### Task 10.2: 追加 SRS 服务到 Docker Compose

**文件**: `docker/docker-compose.yml`（修改现有文件）

**操作**:
1. 在 `services:` 下新增 `srs-server` 服务定义
2. 映射端口: `1935:1935`(RTMP), `8080:8080`(FLV), `8088:8088`(HLS), `1985:1985`(API)
3. 挂载配置: `../srs/srs.conf:/usr/local/srs/conf/srs.conf`
4. 挂载 HLS 输出: `./hls-data:/usr/local/srs/objs/nginx/html/hls`
5. 加入 `chatroom-net` 网络
6. 在 `volumes:` 下新增 `hls-data:` 卷

**新增内容**:
```yaml
  srs-server:
    image: ossrs/srs:5
    container_name: chatroom-srs
    ports:
      - "1935:1935"
      - "8080:8080"
      - "8088:8088"
      - "1985:1985"
    volumes:
      - ../srs/srs.conf:/usr/local/srs/conf/srs.conf:ro
      - hls-data:/usr/local/srs/objs/nginx/html
    networks:
      - chatroom-net
    restart: unless-stopped
```

**验证**: `docker compose -f docker/docker-compose.yml config` 无语法错误

---

### Task 10.3: 追加 stream_records 表到数据库初始化

**文件**: `api-server/src/services/database.js`

**操作**:
1. 找到 `initDatabase()` 函数
2. 在 `room_members` 建表语句之后，追加 `stream_records` 的 `CREATE TABLE IF NOT EXISTS` 语句
3. DDL 内容见 RULES.md 第 3.1 节

**验证**: 启动 `node src/server.js`，检查日志中 MySQL 无报错

---

### Task 10.4: 创建 streamService.js 业务逻辑

**文件**: `api-server/src/services/streamService.js`（新建）

**实现内容**:
1. `const pool = require('./database')`
2. `handlePublish(roomId, streamKey, protocol)` — 推流开始处理：
   - `UPDATE rooms SET state='living', online_count=0 WHERE id=?`
   - `INSERT INTO stream_records (room_id, stream_key, protocol, started_at) VALUES (?, ?, ?, NOW())`
   - 返回 `{ success: true, recordId }`
3. `handleUnpublish(roomId)` — 推流结束处理：
   - `UPDATE rooms SET state='idle' WHERE id=?`
   - `UPDATE stream_records SET ended_at=NOW(), duration_sec=TIMESTAMPDIFF(SECOND, started_at, NOW()) WHERE room_id=? AND ended_at IS NULL`
   - 返回 `{ success: true }`
4. `getStreamInfo(roomId)` — 查询推流状态：
   - 查询 `rooms` 表获取 `state`
   - 若 `state='living'`，构造 `streamUrls` 对象（HLS/FLV 地址）
   - 返回 `{ success: true, data: { isLiving, streamUrls } }`
5. `getLatestStream(roomId)` — 查询最近一次推流记录
6. `module.exports = { handlePublish, handleUnpublish, getStreamInfo, getLatestStream }`

**关键要点**:
- 使用参数化查询，禁止字符串拼接 SQL
- `handleUnpublish` 中 `ended_at IS NULL` 条件确保只更新当前未结束的记录
- HLS URL 格式: `http://localhost:8088/hls/room_{roomId}.m3u8`
- FLV URL 格式: `http://localhost:8080/live/room_{roomId}.flv`

---

### Task 10.5: 创建 stream.js 路由

**文件**: `api-server/src/routes/stream.js`（新建）

**实现内容**:
1. `const express = require('express')` + `const router = express.Router()`
2. `const streamService = require('../services/streamService')`
3. `POST /callback` — SRS 推流回调：
   - 解析 `req.body.action`
   - 从 `req.body.stream` 提取 roomId: `parseInt(req.body.stream.replace('room_', ''), 10)`
   - 校验 roomId 是否有效（正整数）
   - 若 `action === 'on_publish'`，调用 `streamService.handlePublish()`，然后通过 `req.app.get('io')` 获取 Socket.io 实例广播 `stream-started`
   - 若 `action === 'on_unpublish'`，调用 `streamService.handleUnpublish()`，广播 `stream-ended`
   - 返回 `{ code: 0 }`（SRS 要求的响应格式）
4. `GET /rooms/:roomId/stream/info` — 前端获取推流信息：
   - 调用 `streamService.getStreamInfo(roomId)`
   - 返回标准 JSON 格式 `{ success: true, data: {...} }`
5. `module.exports = router`

**关键要点**:
- ⚠️ SRS 回调端点**不能**放在 `/api` 前缀下（已在 RULES.md 配置为 `/api/stream/callback`，需与 server.js 挂载路径核对）。因 server.js 挂载为 `app.use('/api', apiRoutes)`，而 stream 路由是独立 Router，挂载时用 `app.use('/api/stream', streamRoutes)` 即可
- `req.app.get('io')` 需要在 server.js 中设置 `app.set('io', io)`

---

### Task 10.6: 修改 server.js 挂载 stream 路由并注入 io

**文件**: `api-server/src/server.js`

**操作**:
1. 在最顶部引入区域新增: `const streamRoutes = require('./routes/stream')`
2. 在 `app.use('/api', apiRoutes)` 之后新增: `app.use('/api/stream', streamRoutes)`
3. 在 `io` 创建之后、`server.listen` 之前新增: `app.set('io', io)`（使路由中能通过 `req.app.get('io')` 获取 Socket.io 实例）

**验证**: 启动 `node src/server.js`，检查日志显示路由注册成功

---

### Task 10.7: 修改 socket.js 追加推流事件广播

**文件**: `api-server/src/services/socket.js`

**操作**:
1. 确保 `setupSocket(io)` 函数中 `io` 可通过某种方式暴露给外部（当前已有 `io` 变量）
2. 不需要新增 WebSocket 事件（SRS 回调在 HTTP 路由中触发广播）
3. 验证现有广播机制：`io.to('room_{roomId}').emit('stream-started', data)` 可正常工作

**说明**: 推流事件广播在 `stream.js` 路由中通过 `req.app.get('io')` 完成，本文件无需修改。此任务为验证性任务。

---

### Task 10.8: 启动 SRS 容器并验证

**操作**:
1. 确保所有 Docker 服务已停止
2. 启动所有服务: `docker compose -f docker/docker-compose.yml up -d`
3. 检查 srs-server 状态: `docker logs chatroom-srs`
4. 检查端口监听: SRS 应监听 1935/8080/8088/1985

**验证**:
- `docker ps` 显示 chatroom-srs 为 running 状态
- `docker logs chatroom-srs` 无错误信息
- 浏览器访问 `http://localhost:8080` 返回 404（无流时正常行为，证明 HTTP Server 在工作）
- 浏览器访问 `http://localhost:1985/api/v1/versions` 返回 SRS 版本 JSON

---

### Task 10.9: OBS 推流测试

**操作**:
1. 确保 Node.js API Server 在运行 (`node src/server.js`)
2. 打开 OBS Studio → 设置 → 推流
3. 服务: 自定义
4. 服务器: `rtmp://localhost:1935/live`
5. 串流密钥: `room_1`（注意：必须是 `room_{roomId}` 格式，对应数据库中的房间 ID）
6. 添加一个视频源（如屏幕捕获或媒体源）
7. 点击"开始推流"

**验证**:
- OBS 状态栏显示绿色，推流成功
- Node.js 控制台输出 `streamService: room 1 publish started`
- 数据库 `stream_records` 表新增一条记录
- 数据库 `rooms` 表中 room_1 的 `state` 变为 `living`
- 浏览器访问 `http://localhost:8088/hls/room_1.m3u8` 返回 m3u8 内容（非 404）

---

### Task 10.10: 改造 VideoPlayer 对接真实推流

**文件**: `frontend/src/components/player/VideoPlayer.vue`（修改）

**操作**:
1. 新增 props: `streamUrl?: string`
2. 改造逻辑: 当 `streamUrl` 有值时，替换 `demoStream` 播放真实流
3. 当 `streamUrl` 为空时，回退到现有 demoStream（开发保底）
4. 保留现有的 `videoError` fallback 画面

**关键改动**:
```typescript
const props = defineProps<{
  streamUrl?: string
}>()

const actualStream = computed(() => props.streamUrl || demoStream)
```

**验证**: 
- 无推流时显示 demo 流（backward compatible）
- 有推流时通过 RoomView 传入 streamUrl 即可播放真实流

---

### Task 10.11: 改造 RoomView 集成推流状态

**文件**: `frontend/src/views/RoomView.vue`（修改）

**操作**:
1. 新增响应式变量 `streamUrl`、`isStreaming`
2. 进入房间时调用 `GET /api/rooms/:roomId/stream/info` 获取推流状态
3. 若 `isLiving === true`，设置 `streamUrl` 为 HLS 地址
4. 监听 WebSocket `stream-started` → 设置 `streamUrl`、`isStreaming=true`
5. 监听 WebSocket `stream-ended` → 清除 `streamUrl`、`isStreaming=false`
6. 将 `streamUrl` 传递给 `<VideoPlayer :streamUrl="streamUrl" />`

**验证**: 先启动项目，再推流后 RoomView 自动切换到真实视频流

---

### Task 10.12: 全链路端到端验证

**操作**:
1. 完整启动环境: Docker(MySQL+Redis+SRS) + Node.js + Vite
2. 打开前端 http://localhost:5173 → 登录 → 进入 room_1
3. OBS 推流 `rtmp://localhost:1935/live` key=`room_1`
4. 观察前端视频画面是否切换为 OBS 推流内容
5. 停止 OBS 推流 → 观察前端视频是否回退到等待画面

**验证**:
- 推流 → 前端自动切换视频 ✅
- 断流 → 前端自动回退 ✅
- 房间状态 DB 记录正确 ✅
- stream_records 表记录完整 ✅

---

*所有任务完成后，运行 TESTS.md 中的全部测试用例。*
