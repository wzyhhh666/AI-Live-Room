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
| **开发语言** | TypeScript / JavaScript |
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

---

## 核心功能

### 🔐 用户系统
- **简洁登录**：仅输入昵称即可登录，自动完成注册流程
- **角色区分**：三级权限体系 —— 观众(0) / 主播(1) / 管理员(2)

### 📺 直播间管理
- **创建/关闭直播间**：主播可随时开启或关闭直播间
- **大厅展示**：实时展示正在直播的房间（`state=living` 过滤）
- **在线统计**：实时显示各直播间在线人数和点赞数

### 🎥 WebRTC 实时推流
- **WHIP 协议推流**：通过标准 WHIP 协议将视频流推送至 SRS 服务器
- **H.264 编码优先**：使用 `setCodecPreferences` 优先选择 H.264 编码格式
- **ICE 连接检测**：内置 ICE 连接超时检测机制

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
- **互动区域**：弹幕输入区 + 礼物面板 + 点赞按钮

### 🛡️ 安全防护
- **频率限制**：API 层限流，防止刷屏和恶意送礼
- **敏感词过滤**：服务端 523 词库自动过滤违规内容

### 🎨 赛博朋克 UI
- **暗色主题**：主色调 `#0F0F23`
- **霓虹渐变**：赛博朋克风格渐变色彩体系
- **丰富动画**：流畅的过渡动画和微交互效果

---

## 系统架构

```
┌─────────────────────────────────────────────────────────────────┐
│                         用户层 (Client)                          │
│  ┌──────────────┐  ┌──────────────┐                            │
│  │  观众端       │  │  主播端       │                            │
│  │  RoomView    │  │  StudioView  │                            │
│  └──────┬───────┘  └──────┬───────┘                            │
└─────────┼─────────────────┼─────────────────────────────────────┘
          │  HTTP/REST      │  WebRTC(WHIP)
          ▼                 ▼
┌─────────────────────────────────────────────────────────────────┐
│                      API Server (Node.js)                       │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐        │
│  │ Express  │  │Socket.IO │  │ REST API │  │ 中间件   │        │
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

本项目共设计 **6 张数据表**。

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
| effect_type | ENUM | 特效类型 |

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
AI-Live-Room/
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
│   │   ├── router/index.ts          # 路由配置 + 导航守卫
│   │   ├── App.vue                  # 根组件
│   │   ├── main.ts                  # 应用入口
│   │   └── style.css                # 全局样式
│   └── package.json
├── api-server/                      # Node.js 后端服务
│   ├── src/
│   │   ├── server.js                # 入口文件
│   │   ├── routes/                  # 路由层
│   │   │   ├── api.js               # RESTful API
│   │   │   ├── stream.js            # 流媒体 API
│   │   │   └── health.js            # 健康检查
│   │   └── services/                # 业务服务层
│   │       ├── socket.js            # Socket.IO 事件处理
│   │       ├── room.js              # 房间 CRUD + 点赞计数
│   │       ├── gift.js              # 礼物发送 + Redis 排行
│   │       ├── streamService.js     # 流媒体生命周期管理
│   │       ├── database.js          # MySQL 连接池 + 建表
│   │       ├── redisBridge.js       # Redis Pub/Sub 桥接
│   │       ├── sensitiveFilter.js   # 敏感词过滤
│   │       ├── rateLimiter.js       # 频率限制
│   │       └── user.js              # 用户认证
│   └── package.json
├── srs/srs.conf                     # SRS 流媒体服务器配置
├── docker/                          # Docker 部署配置
│   ├── docker-compose.yml           # 服务容器编排
│   ├── Dockerfile                   # 应用容器镜像
│   └── init-sql/01_init.sql         # 数据库初始化脚本
├── docs/                            # 项目提交文档
│   ├── README.md                    # 项目介绍文档
│   ├── 部署文档.md                  # 部署运行指南
│   ├── 答辩PPT.md                   # 答辩演示文稿(12页)
│   ├── AI协作报告.md                # AI辅助开发报告
│   └── 自我评价与反思.md            # 自评与反思
└── README.md                        # 本文件
```

---

## 快速开始

### 环境要求

| 依赖 | 最低版本 | 说明 |
|------|----------|------|
| Node.js | 18+ | JavaScript 运行时 |
| npm | 9+ | 包管理器 |
| Docker Desktop | 最新版 | 容器化部署 |

### 第一步：启动基础设施

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

---

## API 接口文档

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/auth/login` | 用户登录 |
| GET | `/api/rooms` | 房间列表 |
| POST | `/api/rooms` | 创建房间 |
| GET | `/api/rooms/:roomId` | 房间详情 |
| PUT | `/api/rooms/:roomId/close` | 关闭房间 |
| POST | `/api/rooms/:roomId/danmaku` | 发送弹幕 |
| GET | `/api/gifts` | 礼物列表 |
| POST | `/api/rooms/:roomId/gift` | 送礼物 |
| GET | `/api/rooms/:roomId/gift/rank` | 礼物排行 |
| POST | `/api/stream/rooms/:roomId/stream/start` | 开始推流 |
| POST | `/api/stream/rooms/:roomId/stream/stop` | 停止推流 |
| GET | `/api/stream/rooms/:roomId/stream/info` | 流信息 |
| POST | `/api/stream/callback` | SRS 回调 |

## Socket.IO 事件列表

### 客户端 → 服务端

| 事件名 | 说明 |
|--------|------|
| `join-room` | 加入直播间 |
| `send-danmaku` | 发送弹幕 |
| `send-gift` | 发送礼物 |
| `send-like` | 发送点赞 |
| `typing` | 正在输入 |

### 服务端 → 客户端

| 事件名 | 说明 |
|--------|------|
| `stats-update` | 统计数据更新 |
| `new-gift` | 新礼物通知 |
| `new-danmaku` | 新弹幕消息 |
| `user-typing` | 输入提示 |
| `joined-room` | 加入通知 |
| `gift-rank-update` | 排行榜更新 |
| `stream-started` | 推流开始 |
| `stream-ended` | 推流结束 |

---

## 部署方案

### Docker 一键部署（推荐）

```bash
docker compose -f docker/docker-compose.yml up -d --build
```

这将启动全部服务：
- `mysql` — MySQL 8.0 数据库
- `redis` — Redis 7 缓存服务
- `srs-server` — SRS 流媒体服务器
- `nginx` — Nginx 反向代理
- `api-server` — Node.js 后端 API
- `frontend` — Vue 3 前端应用

---

## 许可证

本项目为课程作业项目，仅供学习和研究使用。
