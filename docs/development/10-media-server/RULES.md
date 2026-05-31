# 模块 10 — 流媒体服务器部署与集成 开发规则

> **前置阅读**: [全局主规则](../00-master-rules/MASTER_RULES.md) | [架构设计](../../ARCHITECTURE_DESIGN.md#41-流媒体服务器层-srs)
> **配套文档**: [TASKS.md](./TASKS.md)

---

## 1. 模块定位

部署 SRS (Simple Realtime Server) 流媒体服务器，并在 Node.js API Server 中实现推流回调与房间状态同步，打通 "OBS 推流 → SRS 接入 → HLS 分发 → 前端播放" 完整链路。

**为什么是 Phase 1**: 流媒体层是直播项目的核心基础设施，当前项目缺失视频推拉流能力，前端 VideoPlayer 只能播放第三方 Demo 流。必须最先部署。

---

## 2. 文件变更清单

| 操作 | 文件路径 | 说明 |
|------|----------|------|
| **新建** | `srs/srs.conf` | SRS 流媒体服务器配置 |
| **修改** | `docker/docker-compose.yml` | 新增 srs-server 服务 + HLS 数据卷 |
| **新建** | `api-server/src/routes/stream.js` | 推流回调路由（on_publish/on_unpublish） |
| **新建** | `api-server/src/services/streamService.js` | 推流业务逻辑（状态同步 + stream_records） |
| **修改** | `api-server/src/server.js` | 挂载 stream 路由 + 注册 streamService |
| **修改** | `api-server/src/services/socket.js` | 追加 stream-started/stream-ended 广播 |
| **修改** | `api-server/src/services/database.js` | 追加 stream_records 建表 DDL |
| **新建** | `srs/callback-server/` | SRS HTTP 回调目标（不创建，复用 Node.js 路由） |

---

## 3. 数据模型

### 3.1 stream_records 表（新建）

```sql
CREATE TABLE IF NOT EXISTS stream_records (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    room_id INT NOT NULL,
    stream_key VARCHAR(255) NOT NULL COMMENT '推流密钥',
    protocol ENUM('rtmp', 'srt', 'webrtc') DEFAULT 'rtmp',
    resolution VARCHAR(20) COMMENT '分辨率 1920x1080',
    bitrate INT COMMENT '码率 kbps',
    fps INT COMMENT '帧率',
    started_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    ended_at TIMESTAMP NULL,
    duration_sec INT DEFAULT 0,
    dvr_path VARCHAR(500) COMMENT '录制文件路径',
    INDEX idx_room_time (room_id, started_at),
    FOREIGN KEY (room_id) REFERENCES rooms(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='推流记录表';
```

### 3.2 房间状态变更

推流开始/结束时，需要通过 WebSocket 广播房间状态变化：

| 事件 | payload | 触发条件 |
|------|---------|----------|
| `stream-started` | `{ roomId, streamUrl, timestamp }` | SRS on_publish 回调成功 |
| `stream-ended` | `{ roomId, timestamp }` | SRS on_unpublish 回调成功 |

---

## 4. SRS 配置规范

### 4.1 配置文件位置

`d:\ai直播项目\srs\srs.conf`（项目根目录下的 srs 目录，与 docker-compose 中 volumes 映射一致）

### 4.2 配置模板

```nginx
listen              1935;
max_connections     1000;
daemon              off;

# HLS 配置
hls {
    enabled         on;
    hls_fragment    3;
    hls_window      15;
    hls_path        ./objs/nginx/html/hls;
    hls_m3u8_file   [app]/[stream].m3u8;
    hls_ts_file     [app]/[stream]-[seq].ts;
}

# HTTP-FLV 低延迟拉流
http_server {
    enabled         on;
    listen          8080;
    dir             ./objs/nginx/html;
}

# HTTP API（回调 + 管理）
http_api {
    enabled         on;
    listen          1985;
    crossdomain     on;
}

# 推流回调配置
vhost __defaultVhost__ {
    # 当主播开始推流时，SRS 回调 Node.js API
    http_hooks {
        enabled         on;
        on_publish      http://host.docker.internal:3000/api/stream/callback;
        on_unpublish    http://host.docker.internal:3000/api/stream/callback;
    }
}
```

### 4.3 关键约束

- ⚠️ `http_hooks` 的 URL 在 Docker 容器内必须用 `host.docker.internal`（Windows/Mac）访问宿主机 Node.js 端口 3000
- ⚠️ `crossdomain` 不带下划线（SRS 的正确指令名）
- ⚠️ HLS 分片路径 `hls_path ./objs/nginx/html/hls` 是 SRS 容器内的路径，需通过 docker-compose volumes 映射出来

---

## 5. API 接口规范

### 5.1 SRS 回调 — `POST /api/stream/callback`

**用途**: 接收 SRS 的 on_publish / on_unpublish 事件

**请求体** (SRS 原生格式):
```json
{
  "action": "on_publish",
  "client_id": 123,
  "ip": "192.168.1.100",
  "vhost": "__defaultVhost__",
  "app": "live",
  "stream": "room_1",
  "param": "",
  "tcUrl": "rtmp://localhost:1935/live",
  "stream_url": "/live/room_1",
  "stream_id": "vid-xxx"
}
```

**处理逻辑**:
1. 解析 `stream` 字段，提取 roomId：`parseInt(stream.replace('room_', ''), 10)`
2. 如果 `action === 'on_publish'`:
   - 更新 `rooms` 表: `state='living'`, `online_count=1`
   - INSERT 一条 `stream_records` 记录
   - 通过 socket.io 向房间广播 `stream-started`
3. 如果 `action === 'on_unpublish'`:
   - 更新 `stream_records`: 写入 `ended_at`, 计算 `duration_sec`
   - 更新 `rooms` 表: `state='idle'`
   - 通过 socket.io 向房间广播 `stream-ended`
4. 返回 HTTP 200 (SRS 要求返回 200 才认为成功，非零状态码 SRS 会断开推流)

**响应**: `{ "code": 0 }`  (SRS 要求 `code: 0` 表示成功)

### 5.2 获取推流信息 — `GET /api/rooms/:roomId/stream/info`

**用途**: 前端获取当前房间的推流地址和状态

**响应**:
```json
{
  "success": true,
  "data": {
    "isLiving": true,
    "streamUrls": {
      "hls": "http://localhost:8088/hls/room_1.m3u8",
      "flv": "http://localhost:8080/live/room_1.flv"
    }
  }
}
```

---

## 6. 集成规则

| 规则 | 内容 |
|------|------|
| **不要修改现有路由逻辑** | `stream.js` 是独立路由文件，通过 `server.js` 新增 `app.use('/api/stream', streamRoutes)` 挂载 |
| **复用 database.js 连接池** | 通过 `const pool = require('../services/database')` 获取连接 |
| **推流密钥命名** | 统一使用 `room_{roomId}` 格式，即推流 URL 为 `rtmp://localhost:1935/live/room_1` |
| **Docker Compose 文件不变结构** | 保持 compose 文件在 `docker/` 子目录下，不加 srs-server 到现有 compose，而是在项目根目录新建独立 compose，或追加到现有 compose |
| **SRS 访问宿主机** | Windows Docker 使用 `host.docker.internal`，Linux 使用 `172.17.0.1` 或 `--network host` |

---

## 7. 前端集成要点

### 7.1 VideoPlayer 多协议播放

播放优先级：HLS（CORS 兼容） > HTTP-FLV（低延迟）

- 进入房间后，通过 `GET /api/rooms/:roomId/stream/info` 获取 `streamUrls`
- 若 `isLiving === false`，显示等待画面（已有 fallback 逻辑）
- 若 `isLiving === true`，创建 `<video>` 元素加载 HLS m3u8

### 7.2 流结束处理

- 监听 WebSocket `stream-ended` 事件
- 停止播放，显示 "主播已下播" 画面
- 监听 WebSocket `stream-started` 事件
- 自动重新加载播放

---

## 8. 关键约束

- ⚠️ **stream name 必须为 `room_{roomId}` 格式**：SRS 回调中 `stream` 是字符串如 `room_1`，运行时通过 `parseInt(s.replace('room_', ''))` 提取 roomId
- ⚠️ **SRS 回调要求 HTTP 200 + `{ code: 0 }`**：返回非 200 状态码或非 `code:0` 会导致 SRS 拒绝推流或重试
- ⚠️ **HLS 跨域**：SRS 的 `http_server` 默认不返回 CORS 头，前端播放 HLS 需要通过同源或添加 CORS 配置；由于开发阶段前端 Vite 和 SRS 不同端口，需要在 SRS 配置中加 CORS，或前端通过 Vite proxy 代理
- ⚠️ **Docker 网络**：SRS 容器需要与现有 `chatroom-net` 在同一网络才能访问 MySQL/Redis（虽然当前 SRS 不访问 DB，但保持网络一致性好）

---

*遵循以上规则和全局主规则进行开发。*
