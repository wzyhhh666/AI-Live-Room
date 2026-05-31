# AI Live Room — 开发文档索引

> **最后更新**: 2026-05-24

## 📂 文档结构

```
docs/development/
├── 00-master-rules/        ← 📋 全局主规则（开发前必读）
├── 01-backend-gift-system/ ← 🎁 后端礼物系统
├── 02-frontend-gift-system/← 🎁 前端礼物系统
├── 03-backend-room-crud/   ← 🏠 后端房间管理
├── 04-frontend-room-list-video/← 📺 前端房间列表+视频播放器
├── 05-backend-rate-limiter/← 🚦 后端弹幕限流器
├── 06-backend-sensitive-filter/← 🔍 后端敏感词过滤
├── 07-frontend-danmaku-canvas/← 🎨 前端弹幕Canvas渲染
├── 08-frontend-settings-theme/← ⚙ 前端设置面板+主题系统
├── 09-electron-packaging/  ← 📦 Electron打包分发
├── 10-media-server/        ← 🎬 流媒体服务器部署与集成 [Phase 1]
├── 11-cpp-signal-server/   ← ⚡ C++ Signal Server 集成 [Phase 2 ✅]
├── 12-function-enhancement/← ✨ 功能完善 [Phase 3 ✅]
└── 13-production-hardening/← 🛡️ 生产加固 [Phase 4 当前]
```

## 🗺️ 开发路线图

```
Phase 0 ✅ 已完成 (模块 01-09)
开始
 │
 ├── [01] 后端礼物系统      ← P0: 核心功能 ✅
 │     └── [02] 前端礼物系统  ← P0: 核心功能 ✅
 │
 ├── [03] 后端房间管理      ← P0: 核心功能 ✅
 │     └── [04] 前端房间列表+视频 ← P0: 核心功能 ✅
 │
 ├── [05] 后端弹幕限流器    ← P1: 增强功能 ✅
 │     └── [06] 后端敏感词过滤 ← P1: 增强功能 ✅
 │
 ├── [07] 前端弹幕Canvas    ← P1: 增强功能 ✅
 │
 ├── [08] 前端设置面板+主题  ← P1: 增强功能 ✅
 │
 └── [09] Electron打包分发  ← P2: 分发部署 ✅

Phase 1 🎬 流媒体层 (模块 10) [✅ 完成]
 │
 └── [10] 流媒体服务器部署   ← P0: SRS + 推流回调 + VideoPlayer改造

Phase 2 ⚡ C++ 实时层 [✅ 完成]
 │
 └── [11] C++ Signal Server 集成 ← P0+P1: Redis Sub/Pub + 全链路打通

Phase 3 ✨ 功能完善 [✅ 完成]
 │
 └── [12] 表情 + 敏感词库 + 推流密钥 + 房间统计

Phase 4 🛡️ 生产加固 [← 当前进行中]
 │
 └── [13] Nginx + 健康检查 + 错误处理 + 参数校验 + K8s + 压测
```

## 📋 每模块文档约定

| 文档 | 用途 | 阅读顺序 |
|------|------|----------|
| `RULES.md` | 开发规则：文件清单、接口签名、数据模型、集成约束 | 开发前必读 |
| `TASKS.md` | 开发任务：按序号执行的实施步骤 | 开发中参照 |
| `TESTS.md` | 测试文档：逐条测试用例和通过标准 | 完成后执行 |

## ⚡ 快速验证命令

```bash
# 后端验证
cd api-server && node src/server.js

# 前端验证
cd frontend && npx vue-tsc --noEmit && npx vite build
```
