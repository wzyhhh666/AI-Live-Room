# AI Live Room Platform (AI直播平台)

<p align="center">
  <strong>基于 WebRTC + Socket.IO 的实时直播互动平台</strong>
</p>


---

## 📋 目录

- [项目简介](#项目简介)
- [技术栈](#技术栈)
- [核心功能](#核心功能)
- [系统架构](#系统架构)
- [数据库设计](#数据库设计)
- [目录结构](#目录结构)
- [快速开始](#快速开始)
- [API 接口文档](#api-接口文档)
- [Socket.IO 事件列表](#socketio-事件列表)
- [部署方案](#部署方案)
- [项目亮点](#项目亮点)

---

## 项目简介

AI Live Room（AI直播平台）是一个**实时直播互动平台**，采用现代化的前后端分离架构，集成了 **WebRTC 实时推流、多协议视频播放、实时弹幕、礼物系统、点赞互动**等完整直播功能。项目采用赛博朋克风格 UI 设计，提供沉浸式的视觉体验。

### 版本信息

| 项目 | 说明 |
|------|------|
| **项目名称** | AI Live Room Platform |
| **当前版本** | v1.0.0 |
| **开发语言** | TypeScript / JavaScript / C++ |
| **运行环境** | Node.js 18+ / Docker |

---

## 技术栈

### 前端技术

| 技术 | 版本 | 用途 |
|------|------|------|
| Vue.js | 3.4 | 渐进式前端框架 |
| TypeScript | 5.3 | 类型安全的超集语言 |
| Vite | 5 | 新一代前端构建工具 |
| Pinia | 2 | 状态管理 |
| Vue Router | 4 | 路由管理 |
| TailwindCSS | 3.4 | 原子化 CSS 框架 |
| mpegts.js | - | HTTP-FLV 低延迟播放 |
| Socket.IO Client | 4.7 | 实时双向通信 |

### 后端技术

| 技术 | 版本 | 用途 |
|------|------|------|
| Node.js | 18+ | 运行时环境 |
| Express | 4 | Web 服务框架 |
| Socket.IO | 4.7 | WebSocket 实时通信 |
| MySQL2 | 3.6 | MySQL 数据库驱动 |
| ioredis | 5.3 | Redis 客户端 |

### 流媒体服务

| 技术 | 版本 | 协议支持 |
|------|------|----------|
| SRS (Simple Realtime Server) | 5 | WebRTC WHIP/WHEP / HTTP-FLV / HLS |
| mpegts.js | - | FLV 格式播放器 |
| HTML5 Video | - | HLS 原生播放 |

### 基础设施

| 组件 | 版本 | 说明 |
|------|------|------|
| Docker Compose | 3.8 | 容器编排 |
| MySQL | 8.0 | 关系型数据库 |
| Redis | 7 Alpine | 缓存与消息队列 |
| Nginx | - | 反向代理与负载均衡 |
| Electron | 42 | 桌面客户端封装（可选） |

### 高性能备选方案

| 组件 | 技术栈 | 特性 |
|------|---------|------|
| C++ 后端服务器 | epoll + 线程池 + 定时器轮 | 高并发事件驱动架构 |
| Redis Pub/Sub | 订阅发布模式 | 实时消息分发 |
| MySQL 连接池 | 连接复用 | 数据库性能优化 |

---

## 核心功能

### 🔐 用户系统
- **简洁登录**：仅输入昵称即可登录，自动完成注册流程
- **角色区分**：三级权限体系 —— 观众(0) / 主播(1) / 管理员(2)
- **Token 认证**：基于 Token 的身份验证机制

### 📺 直播间管理
- **创建/关闭直播间**：主播可随时开启或关闭直播间
- **大厅展示**：实时展示正在直播的房间（`state=living` 过滤）
- **在线统计**：实时显示各直播间在线人数和点赞数

### 🎥 WebRTC 实时推流
- **WHIP 协议推流**：通过标准 WHIP 协议将视频流推送至 SRS 服务器
- **H.264 编码优先**：使用 `setCodecPreferences` 优先选择 H.264 编码格式
- **ICE 连接检测**：内置 ICE 连接超时检测机制，保障连接稳定性

### 📹 多格式播放
- **HTTP-FLV 播放**：基于 mpegts.js 实现 **低延迟**（1~3s）播放体验
- **HLS 播放**：原生 HTML5 video 标签支持，兼容性更广
- **自动降级切换**：FLV 不可用时自动降级至 HLS 协议

### 💬 弹幕系统
- **实时广播**：基于 Socket.IO 实现毫秒级弹幕广播
- **Canvas 渲染**：使用 Canvas API 渲染滚动弹幕动画效果
- **敏感词过滤**：内置 **523 词库**的敏感词过滤系统
- **防重复机制**：防止相同弹幕内容重复显示

### 🎁 礼物系统
- **6 种礼物类型**：荧光棒 / 点赞 / 鲜花 / 跑车 / 火箭 / 嘉年华
- **4 种特效类型**：普通(normal) / 爆炸(explosion) / 礼物雨(rain) / 火箭(rocket)
- **Redis ZSET 排行榜**：使用 `ZINCRBY` / `ZREVRANGE` 实现实时排行
- **MySQL Fallback**：Redis 不可用时自动降级到 MySQL 存储

### 👍 点赞功能
- **Heart 按钮**：心形按钮实时点赞交互
- **DB 计数**：`like_count` 字段持久化存储点赞数据
- **广播同步**：`stats-update` 事件实时同步点赞状态

### 🎙️ 主播工作室 (StudioView)
- **摄像头预览**：实时本地摄像头画面预览
- **一体化面板**：聊天 / 礼物 / 弹幕面板集成
- **推流控制**：一键开始/停止推流操作

### 👁️ 观众端 (RoomView)
- **视频播放器**：FLV/HLS 双协议自适应播放
- **LIVE 标签**：醒目的直播状态标识
- **主播信息展示**：主播头像、昵称等信息
- **互动区域**：弹幕输入区 + 礼物面板 + 点赞按钮

### 🛡️ 安全防护
- **频率限制**：API 层限流，防止刷屏和恶意送礼
- **敏感词过滤**：服务端 523 词库自动过滤违规内容

### 🎨 赛博朋克 UI
- **暗色主题**：主色调 `#0F0F23`，护眼且沉浸感强
- **霓虹渐变**：赛博朋克风格渐变色彩体系
- **丰富动画**：流畅的过渡动画和微交互效果
- **主题切换**：支持亮色/暗色主题自由切换

### 💻 Electron 打包
- **桌面客户端**：可选的 Electron 封装方案
- **跨平台**：支持 Windows / macOS / Linux

---

## 系统架构

```
┌─────────────────────────────────────────────────────────────────┐
│                         用户层 (Client)                          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐          │
│  │  观众端       │  │  主播端       │  │  Electron    │          │
│  │  RoomView    │  │  StudioView  │  │  Desktop     │          │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘          │
└─────────┼─────────────────┼─────────────────┼───────────────────┘
          │  HTTP/REST      │  WebRTC(WHIP)   │
          ▼                 ▼                 ▼
┌─────────────────────────────────────────────────────────────────┐
│                      API Server (Node.js)                       │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐        │
│  │ Express  │  │Socket.IO │  │ REST API │  │ 中间件   │        │
│  │ Router   │  │  Server  │  │ Routes   │  │ Auth/Rate│        │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘        │
│       │             │             │             │               │
│  ┌────▼─────────────▼─────────────▼─────────────▼─────┐        │
│  │              业务服务层 (Services)                  │        │
│  │ room │ gift │ stream │ socket │ user │ filter     │        │
│  └──────────────────────┬────────────────────────────┘        │
└─────────────────────────┼─────────────────────────────────────┘
                          │
          ┌───────────────┼───────────────┐
          ▼               ▼               ▼
   ┌────────────┐  ┌────────────┐  ┌────────────┐
   │   MySQL    │  │   Redis    │  │    SRS     │
   │   8.0      │  │   7.x      │  │    5.x     │
   │  (持久存储) │  │ (缓存/排行)│  │ (流媒体)   │
   └────────────┘  └────────────┘  └────────────┘
```

### 数据流说明

| 流向 | 协议 | 说明 |
|------|------|------|
| 主播 → SRS | WebRTC WHIP | H.264 视频编码推流 |
| SRS → 观众 | HTTP-FLV / HLS | 低延迟 / 兼容性播放 |
| 客户端 → API | HTTP/REST | 房间、用户、礼物等 CRUD |
| 客户端 ↔ API | Socket.IO WebSocket | 弹幕、礼物、点赞实时通信 |
| SRS → API | HTTP Callback | 推流开始/结束回调通知 |

---

## 数据库设计

本项目共设计 **6 张数据表**，覆盖用户、房间、弹幕、礼物、成员关系等核心业务域。

### ER 关系图

```
users (1) ───< (N) rooms           (一个用户可以创建多个房间)
rooms  (1) ───< (N) danmaku_messages  (一个房间有多条弹幕)
rooms  (1) ───< (N) gift_record       (一个房间有多条送礼记录)
rooms  (1) ───< (N) room_members      (一个房间有多个成员)
gift_config ──> (N) gift_record       (礼物配置关联送礼记录)
```

### 表结构详情

#### users（用户表）

| 字段名 | 类型 | 说明 |
|--------|------|------|
| id | INT PK | 主键 ID |
| username | VARCHAR | 登录用户名 |
| nickname | VARCHAR | 显示昵称 |
| avatar | VARCHAR | 头像 URL |
| role | TINYINT | 角色：0=观众, 1=主播, 2=管理员 |
| token | VARCHAR | 认证令牌 |

#### rooms（直播间表）

| 字段名 | 类型 | 说明 |
|--------|------|------|
| id | INT PK | 主键 ID |
| room_name | VARCHAR | 房间唯一名称 |
| host_id | INT FK | 主播用户 ID |
| host_name | VARCHAR | 主播昵称 |
| title | VARCHAR | 直播间标题 |
| cover_image | VARCHAR | 封面图片 URL |
| online_count | INT | 在线人数 |
| like_count | INT | 点赞总数 |
| state | ENUM | 状态：idle / living / closed |

#### danmaku_messages（弹幕消息表）

| 字段名 | 类型 | 说明 |
|--------|------|------|
| id | INT PK | 主键 ID |
| room_id | INT FK | 所属房间 ID |
| user_id | INT FK | 发送者用户 ID |
| username | VARCHAR | 发送者昵称 |
| content | TEXT | 弹幕内容 |
| color | VARCHAR | 弹幕颜色 |
| type | ENUM | 类型：normal / gift / system |

#### gift_record（礼物记录表）

| 字段名 | 类型 | 说明 |
|--------|------|------|
| record_id | INT PK | 主键 ID |
| room_id | INT FK | 所属房间 ID |
| sender_id | INT FK | 送礼者 ID |
| sender_name | VARCHAR | 送礼者昵称 |
| receiver_id | INT FK | 接收者 ID |
| gift_id | INT FK | 礼物配置 ID |
| gift_name | VARCHAR | 礼物名称 |
| gift_count | INT | 礼物数量 |
| total_price | DECIMAL | 总价格 |
| effect_type | ENUM | 特效类型：normal / explosion / rain / rocket |

#### gift_config（礼物配置表）

| 字段名 | 类型 | 说明 |
|--------|------|------|
| id | INT PK | 主键 ID |
| gift_name | VARCHAR | 礼物名称 |
| gift_icon | VARCHAR | 礼物图标 (Emoji) |
| price | DECIMAL | 单价 |
| effect_type | ENUM | 特效类型 |
| sort_order | INT | 排序权重 |

#### room_members（房间成员表）

| 字段名 | 类型 | 说明 |
|--------|------|------|
| id | INT PK | 主键 ID |
| room_id | INT FK | 所属房间 ID |
| user_id | INT FK | 用户 ID |
| username | VARCHAR | 用户昵称 |
| join_time | DATETIME | 加入时间 |
| leave_time | DATETIME | 离开时间 |

---

## 目录结构

```
d:\ai直播项目/
├── frontend/                        # Vue 3 前端应用
│   ├── src/
│   │   ├── views/                   # 页面视图组件
│   │   │   ├── LoginView.vue        # 登录页（取名即入）
│   │   │   ├── RoomListView.vue     # 大厅 / 房间列表页
│   │   │   ├── RoomView.vue         # 观众端直播间页面
│   │   │   ├── StudioView.vue       # 主播端工作室页面
│   │   │   └── SettingsView.vue     # 设置页面
│   │   ├── components/              # 可复用组件
│   │   │   ├── player/              # 视频播放相关
│   │   │   │   ├── VideoPlayer.vue  # FLV/HLS 自适应播放器
│   │   │   │   └── DanmakuCanvas.vue # Canvas 弹幕渲染
│   │   │   ├── gift/                # 礼物系统组件
│   │   │   │   ├── GiftPanel.vue    # 礼物面板
│   │   │   │   ├── GiftItem.vue     # 礼物项
│   │   │   │   ├── GiftEffect.vue   # 礼物特效
│   │   │   │   └── GiftRank.vue     # 礼物排行榜
│   │   │   └── room/                # 房间相关组件
│   │   │       └── RoomCard.vue     # 房间卡片
│   │   ├── stores/                  # Pinia 状态管理
│   │   │   ├── user.ts              # 用户认证状态
│   │   │   ├── room.ts              # 房间数据状态
│   │   │   └── gift.ts              # 礼物 + 排行状态
│   │   ├── services/                # API 服务封装
│   │   │   ├── api.ts               # RESTful API + Socket.IO 封装
│   │   │   └── whipClient.ts        # WebRTC WHIP 推流客户端
│   │   ├── router/
│   │   │   └── index.ts             # 路由配置 + 导航守卫
│   │   ├── composables/
│   │   │   └── useTheme.ts          # 主题切换组合式函数
│   │   ├── App.vue                  # 根组件
│   │   ├── main.ts                  # 应用入口
│   │   ├── style.css                # 全局样式
│   │   └── docs/                    # PRD + 技术文档
│   ├── electron/                    # Electron 桌面客户端配置
│   └── package.json
├── api-server/                      # Node.js 后端服务
│   ├── src/
│   │   ├── server.js                # 入口文件：Express + Socket.IO + 中间件
│   │   ├── routes/                  # 路由层
│   │   │   ├── api.js               # RESTful API (rooms/gifts/auth/stream)
│   │   │   ├── stream.js            # 流媒体 API (startStream/stopStream/getStreamInfo)
│   │   │   └── health.js            # 健康检查接口
│   │   ├── services/                # 业务服务层
│   │   │   ├── socket.js            # Socket.IO 事件处理
│   │   │   │                       # (join-room/send-danmaku/send-gift/send-like/typing)
│   │   │   ├── room.js              # 房间 CRUD + 在线人数 + 点赞计数
│   │   │   ├── gift.js              # 礼物发送 + Redis 排行(ZINCRBY/ZREVRANGE)
│   │   │   ├── streamService.js     # 流媒体生命周期管理
│   │   │   │                       # (startStream/handlePublish/handleUnpublish)
│   │   │   ├── database.js          # MySQL 连接池 + 自动建表
│   │   │   ├── redisBridge.js       # Redis Pub/Sub 消息桥接
│   │   │   ├── sensitiveFilter.js   # 敏感词过滤 (523 词库)
│   │   │   ├── rateLimiter.js       # API 频率限制
│   │   │   └── user.js              # 用户认证服务
│   │   ├── middleware/              # 中间件
│   │   └── config/                  # 配置文件
│   └── package.json
├── srs/
│   └── srs.conf                     # SRS 流媒体服务器配置文件
├── docker/                          # Docker 部署配置
│   ├── docker-compose.yml           # 6 个服务容器编排
│   ├── Dockerfile                   # 应用容器镜像构建
│   ├── init-sql/
│   │   └── 01_init.sql              # 数据库初始化脚本
│   └── nginx/                       # Nginx 反向代理配置
├── backend-server/                  # C++ 高性能后端 (备选方案)
│   ├── include/                     # 头文件
│   │   ├── epoll.h                  # epoll 事件驱动模型
│   │   ├── mysql_pool.h             # MySQL 连接池
│   │   ├── redis.h                  # Redis 客户端封装
│   │   ├── thread_pool.h            # 线程池实现
│   │   └── timer_wheel.h            # 定时器轮算法
│   ├── src/                         # 源码实现
│   ├── main.cpp                     # 程序入口
│   └── CMakeLists.txt               # CMake 构建配置
├── chatroom-client/                 # C++ 客户端 (备选)
├── chatroom-server/                 # C++ 服务器 (备选)
├── docs/                            # 项目文档
│   ├── development/                 # 开发规则文档
│   │   ├── RULES/                   # 各模块开发规范 (13个模块)
│   │   ├── TASKS/                   # 任务拆分
│   │   └── TESTS/                   # 测试用例
│   ├── ARCHITECTURE_DESIGN.md       # 架构设计文档
│   ├── TECHNICAL_FRAMEWORK.md       # 技术框架文档
│   └── USER_GUIDE.md                # 用户使用指南
├── scripts/                         # 工具脚本
└── config/k8s/                      # Kubernetes 部署配置
```

---

## 快速开始

### 环境要求

| 依赖 | 最低版本 | 说明 |
|------|----------|------|
| Node.js | 18+ | JavaScript 运行时 |
| npm | 9+ | 包管理器 |
| Docker Desktop | 最新版 | 容器化部署 |
| MySQL | 8.0 | 关系数据库 |
| Redis | 7.x | 缓存数据库 |

### 第一步：启动基础设施

使用 Docker Compose 启动 MySQL、Redis 和 SRS 流媒体服务器：

```bash
docker compose -f docker/docker-compose.yml up -d mysql redis srs-server
```

### 第二步：启动 API Server

```bash
cd api-server
npm install
node src/server.js
```

> 服务默认运行在 **端口 3000**

### 第三步：启动前端开发服务器

```bash
cd frontend
npm install
npx vite --host 0.0.0.0 --port 5173
```

> 前端默认运行在 **端口 5173**

### 第四步：访问应用

打开浏览器访问：**http://localhost:5173**

1. 输入昵称即可登录（自动注册）
2. 以主播身份创建房间并开播
3. 以观众身份进入直播间观看互动

### Docker 一键部署（推荐生产环境）

```bash
docker compose -f docker/docker-compose.yml up -d --build
```

这将启动全部 6 个服务：
- `mysql` — MySQL 8.0 数据库
- `redis` — Redis 7 缓存服务
- `srs-server` — SRS 流媒体服务器
- `nginx` — Nginx 反向代理
- `api-server` — Node.js 后端 API
- `frontend` — Vue 3 前端应用

---

## API 接口文档

### 认证接口

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/auth/login` | 用户登录（输入昵称自动注册） |

### 房间接口

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/rooms` | 获取房间列表（含 living 状态过滤） |
| POST | `/api/rooms` | 创建新直播间 |
| GET | `/api/rooms/:roomId` | 获取房间详情 |
| PUT | `/api/rooms/:roomId/close` | 关闭直播间 |

### 弹幕接口

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/rooms/:roomId/danmaku` | 发送弹幕消息 |

### 礼物接口

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/gifts` | 获取礼物配置列表 |
| POST | `/api/rooms/:roomId/gift` | 发送礼物 |
| GET | `/api/rooms/:roomId/gift/rank` | 获取礼物排行榜 |

### 流媒体接口

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/stream/rooms/:roomId/stream/start` | 开始推流 |
| POST | `/api/stream/rooms/:roomId/stream/stop` | 停止推流 |
| GET | `/api/stream/rooms/:roomId/stream/info` | 获取流信息 |
| POST | `/api/stream/callback` | SRS 回调通知 (publish/unpublish) |

### 健康检查

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/health` | 服务健康状态检查 |

---

## Socket.IO 事件列表

### 客户端 → 服务端（发送事件）

| 事件名 | 参数 | 说明 |
|--------|------|------|
| `join-room` | `{ roomId }` | 加入直播间 |
| `send-danmaku` | `{ content, color }` | 发送弹幕 |
| `send-gift` | `{ giftId, count }` | 发送礼物 |
| `send-like` | - | 发送点赞 |
| `typing` | - | 正在输入提示 |

### 服务端 → 客户端（广播事件）

| 事件名 | 参数 | 说明 |
|--------|------|------|
| `stats-update` | `{ onlineCount, likeCount }` | 房间统计数据更新 |
| `new-gift` | `{ sender, giftName, count }` | 新礼物通知 |
| `new-danmaku` | `{ username, content, color }` | 新弹幕消息 |
| `user-typing` | `{ username }` | 用户正在输入 |
| `joined-room` | `{ username }` | 用户加入通知 |
| `gift-rank-update` | `{ ranks }` | 排行榜更新 |
| `stream-started` | `{ streamUrl }` | 推流开始通知 |
| `stream-ended` | `{ reason }` | 推流结束通知 |

---

## 部署方案

### 开发环境部署

按照[快速开始](#快速开始)章节中的步骤依次启动各服务。

### 生产环境 Docker 部署

```bash
# 构建并启动所有服务
docker compose -f docker/docker-compose.yml up -d --build

# 查看服务状态
docker compose -f docker/docker-compose.yml ps

# 查看日志
docker compose -f docker/docker-compose.yml logs -f api-server
```

### Kubernetes 部署

项目提供 K8s 部署配置，位于 `config/k8s/` 目录：

```bash
kubectl apply -f config/k8s/
```

### Nginx 配置要点

```nginx
location /api/ {
    proxy_pass http://api-server:3000;
}

location /socket.io/ {
    proxy_pass http://api-server:3000;
    proxy_http_version 1.1;
    proxy_set_header Upgrade $http_upgrade;
    proxy_set_header Connection "upgrade";
}
```

---

## 项目亮点

### 1. 多协议流媒体架构

采用 **WebRTC WHIP 推流 + HTTP-FLV/HLS 播放**的多协议方案，兼顾低延迟与兼容性：

- 推流端使用 WebRTC WHIP 标准协议，浏览器原生支持无需插件
- 播放端优先使用 HTTP-FLV（mpegts.js），延迟控制在 **1~3 秒**
- 自动降级到 HLS 协议，保证弱网环境下可用性

### 2. 实时互动体验

- **Socket.IO WebSocket** 实现弹幕、礼物、点赞的毫秒级实时通信
- **Canvas 弹幕引擎**：高性能滚动弹幕渲染，支持自定义颜色和速度
- **Redis ZSET 排行榜**：O(log N) 复杂度的实时礼物排名计算

### 3. 高可用数据存储

- **MySQL** 作为主存储，保证数据持久化和事务一致性
- **Redis** 作为缓存层，提供排行榜和会话数据的高速读写
- **自动降级策略**：Redis 故障时无缝降级到 MySQL，保障服务连续性

### 4. 内容安全体系

- **523 词敏感词库**：服务端自动过滤违规弹幕内容
- **API 频率限制**：防止恶意刷屏和刷礼物攻击
- **Token 身份认证**：所有接口均需鉴权访问

### 5. 赛博朋克 UI 设计

- 暗色主题 (`#0F0F23`) 降低视觉疲劳
- 霓虹渐变色彩营造科技氛围
- 丰富的 CSS 动画和过渡效果提升交互质感
- 支持亮色/暗色主题一键切换

### 6. 可扩展架构

- **C++ 高性能备选后端**：epoll 事件驱动 + 线程池 + 定时器轮
- **Electron 桌面客户端**：可选的桌面应用封装
- **Kubernetes 部署配置**：支持云原生容器编排
- **Docker Compose 编排**：一键启动全栈服务

---

## 许可证

本项目为课程作业项目，仅供学习和研究使用。

---

<p align="center">
  <strong>AI Live Room Platform &copy; 2026</strong>
</p>
