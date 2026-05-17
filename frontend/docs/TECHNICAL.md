# AI直播客户端 - 技术架构文档

## 1. 系统架构总览

### 1.1 架构模式
采用 **Electron Main/Renderer Process** 架构 + **MVVM (Model-View-ViewModel)** 模式

```
┌─────────────────────────────────────────────────────┐
│                  Electron Application                │
│                                                      │
│  ┌──────────────┐    IPC/RPC    ┌────────────────┐ │
│  │   Main       │◄────────────►│   Renderer     │ │
│  │   Process    │              │   Process      │ │
│  │              │              │                │ │
│  │ ┌──────────┐ │              │ ┌────────────┐ │ │
│  │ │Window Mgr│ │              │ │  Vue 3 App  │ │ │
│  │ ├──────────┤ │              │ ├────────────┤ │ │
│  │ │TCP Client│ │              │ │  Pinia Store│ │ │
│  │ ├──────────┤ │              │ ├────────────┤ │ │
│  │ │File System│ │              │ │  Vue Router │ │ │
│  │ └──────────┘ │              │ └────────────┘ │ │
│  └──────────────┘              └────────────────┘ │
│         ▲                              ▲           │
│         │                              │           │
│    ┌────┴────┐                   ┌─────┴─────┐     │
│    │Backend  │                   │  Browser   │     │
│    │Server   │                   │  APIs      │     │
│    │(C++:8900)│                   │(Canvas/WebGL)│   │
│    └─────────┘                   └───────────┘     │
└─────────────────────────────────────────────────────┘
```

---

## 2. 技术栈详细选型

### 2.1 核心框架层

| 技术 | 版本 | 用途 | 选型理由 |
|------|------|------|----------|
| **Electron** | ^28.0 | 桌面应用框架 | 跨平台, 生态成熟 |
| **Vue.js** | ^3.4 | UI框架 | Composition API, 性能优秀 |
| **Vite** | ^5.0 | 构建工具 | 极速HMR, 原生ESM |
| **TypeScript** | ^5.0 | 类型安全 | 开发体验, 减少Bug |

### 2.2 状态管理与路由

| 技术 | 版本 | 用途 |
|------|------|------|
| **Pinia** | ^2.1 | 全局状态管理 |
| **Vue Router** | ^4.2 | SPA路由管理 |
| **vueuse** | ^10.7 | 组合式API工具集 |

### 2.3 UI与样式

| 技术 | 版本 | 用途 |
|------|------|------|
| **Tailwind CSS** | ^3.4 | 原子化CSS框架 |
| **Headless UI** | ^1.7 | 无样式UI组件 |
| **Heroicons** | ^2.0 | SVG图标库 |
| **animate.css** | ^4.1 | CSS动画库 |

### 2.4 图表与可视化

| 技术 | 版本 | 用途 |
|------|------|------|
| **ECharts** | ^5.5 | 数据图表可视化 |
| **canvas-confetti** | ^1.9 | 礼物特效粒子 |
| **@vueuse/core** | ^10.9 | 动画与交互工具 |

### 2.5 通信与网络

| 技术 | 版本 | 用途 |
|------|------|------|
| **ws** | ^8.16 | WebSocket客户端 |
| **net** (Node.js) | 内置 | TCP Socket连接 |
| **protobufjs** | ^7.2 | Protobuf编解码 |

### 2.6 构建与打包

| 技术 | 版本 | 用途 |
|------|------|------|
| **electron-builder** | ^24.9 | 应用打包分发 |
| **electron-vite** | ^2.0 | Electron专用构建 |
| **esbuild** | ^0.20 | JavaScript打包器 |

---

## 3. 项目目录结构

```
frontend/
├── electron/                    # Electron主进程代码
│   ├── main.ts                 # 主进程入口
│   ├── preload.ts              # 预加载脚本
│   ├── ipc/                    # IPC通信处理
│   │   ├── handlers.ts          # IPC处理器注册
│   │   └── channels.ts         # IPC通道定义
│   ├── services/               # 主进程服务
│   │   ├── tcp-client.ts       # TCP连接服务
│   │   ├── websocket.ts        # WebSocket服务
│   │   ├── window-manager.ts   # 窗口管理
│   │   └── updater.ts          # 自动更新
│   └── utils/                  # 工具函数
│       └── logger.ts           # 日志系统
│
├── src/                        # 渲染进程代码 (Vue)
│   ├── main.ts                 # Vue应用入口
│   ├── App.vue                 # 根组件
│   │
│   ├── router/                 # 路由配置
│   │   └── index.ts
│   │
│   ├── stores/                 # Pinia状态仓库
│   │   ├── user.ts             # 用户状态
│   │   ├── room.ts             # 房间状态
│   │   ├── danmaku.ts          # 弹幕状态
│   │   └── gift.ts             # 礼物状态
│   │
│   ├── views/                  # 页面组件
│   │   ├── LoginView.vue       # 登录页
│   │   ├── RoomView.vue        # 直播间主页
│   │   └── SettingsView.vue    # 设置页
│   │
│   ├── components/             # 可复用组件
│   │   ├── common/             # 通用组件
│   │   │   ├── AppHeader.vue   # 顶部导航栏
│   │   │   ├── AppFooter.vue   # 底部控制栏
│   │   │   └── LoadingSpinner.vue
│   │   │
│   │   ├── player/             # 播放器组件
│   │   │   ├── VideoPlayer.vue # 视频播放器
│   │   │   ├── DanmakuCanvas.vue # 弹幕画布
│   │   │   └── GiftEffect.vue  # 礼物特效
│   │   │
│   │   ├── chat/               # 聊天组件
│   │   │   ├── ChatPanel.vue   # 聊天面板
│   │   │   ├── DanmakuInput.vue # 弹幕输入框
│   │   │   ├── UserList.vue    # 用户列表
│   │   │   └── MessageItem.vue # 消息项
│   │   │
│   │   ├── gift/               # 礼物组件
│   │   │   ├── GiftPanel.vue   # 礼物面板
│   │   │   ├── GiftItem.vue    # 礼物项
│   │   │   └── GiftRank.vue    # 礼物排行榜
│   │   │
│   │   └── room/               # 房间组件
│   │       ├── RoomList.vue    # 房间列表
│   │       ├── RoomCard.vue    # 房间卡片
│   │       └── RoomInfo.vue    # 房间信息
│   │
│   ├── composables/            # 组合式函数
│   │   ├── useConnection.ts    # 连接管理
│   │   ├── useDanmaku.ts       # 弹幕逻辑
│   │   ├── useGift.ts          # 礼物逻辑
│   │   └── useTheme.ts         # 主题切换
│   │
│   ├── utils/                  # 工具函数
│   │   ├── protocol.ts         # 协议编解码
│   │   ├── formatter.ts        # 格式化工具
│   │   └── validators.ts       # 验证函数
│   │
│   ├── styles/                 # 全局样式
│   │   ├── main.css            # 入口样式
│   │   ├── variables.css       # CSS变量
│   │   └── animations.css      # 动画定义
│   │
│   └── assets/                 # 静态资源
│       ├── images/             # 图片
│       ├── icons/              # 图标
│       └── sounds/             # 音效
│
├── resources/                  # Electron资源
│   ├── icon.ico                # 应用图标
│   ├── tray-icon.png           # 托盘图标
│   └── splash.png              # 启动画面
│
├── docs/                       # 项目文档
│   ├── PRD.md                  # 产品需求文档
│   └── TECHNICAL.md            # 本技术文档
│
├── tests/                      # 测试文件
│   ├── unit/                   # 单元测试
│   └── e2e/                    # 端到端测试
│
├── package.json                # 项目配置
├── tsconfig.json               # TypeScript配置
├── vite.config.ts              # Vite配置
├── tailwind.config.js          # Tailwind配置
├── electron.vite.config.ts     # Electron Vite配置
└── builder.config.ts           # 打包配置
```

---

## 4. 核心模块设计

### 4.1 通信层架构

#### 4.1.1 双通道通信策略

```typescript
// 通信策略选择逻辑
class ConnectionManager {
  private ws: WebSocket;      // WebSocket (首选)
  private tcp: net.Socket;    // TCP Socket (备用)
  
  async connect() {
    // 优先尝试WebSocket (低延迟, 支持双向推送)
    try {
      await this.connectWebSocket();
    } catch {
      // 回退到TCP Socket (稳定可靠)
      await this.connectTCP();
    }
  }
}
```

#### 4.1.2 消息协议适配

```typescript
// 协议消息格式 (兼容后端C++协议)
interface ProtocolMessage {
  header: {
    magic: number;        // 0x48415443
    version: number;      // 1
    msgType: number;      // 消息类型
    seq: number;          // 序列号
    roomId: number;       // 房间ID
    userId: number;       // 用户ID
    timestamp: number;    // 时间戳
    bodyLen: number;      // Body长度
  };
  body: string;           // JSON/Protobuf数据
}
```

### 4.2 弹幕渲染引擎

#### 4.2.1 Canvas分层渲染

```
┌─────────────────────────────────┐
│ Layer 3: Gift Effects (z-index:3)│ ← 粒子特效层
├─────────────────────────────────┤
│ Layer 2: Top Danmaku (z-index:2)│ ← 顶部弹幕层
├─────────────────────────────────┤
│ Layer 1: Scroll Danmaku (z-index:1)│ ← 滚动弹幕层
├─────────────────────────────────┤
│ Layer 0: Video Background (z-index:0)│ ← 视频底层
└─────────────────────────────────┘
```

#### 4.2.2 弹幕对象池

```typescript
// 对象池优化 - 避免频繁GC
class DanmakuPool {
  private pool: Danmaku[] = [];
  private maxPoolSize = 500;
  
  acquire(): Danmaku {
    return this.pool.pop() || new Danmaku();
  }
  
  release(danmaku: Danmaku) {
    if (this.pool.length < this.maxPoolSize) {
      danmaku.reset();
      this.pool.push(danmaku);
    }
  }
}
```

### 4.3 状态管理架构

#### 4.3.1 Pinia Store设计

```typescript
// stores/user.ts
export const useUserStore = defineStore('user', () => {
  // State
  const userInfo = ref<UserInfo | null>(null);
  const isLoggedIn = ref(false);
  const token = ref<string>('');
  
  // Actions
  async function login(username: string, password: string) { ... }
  async function logout() { ... }
  
  // Getters
  const displayName = computed(() => userInfo.value?.nickname || 'Guest');
  
  return { userInfo, isLoggedIn, token, login, logout, displayName };
});
```

---

## 5. 性能优化策略

### 5.1 渲染性能

| 优化点 | 方案 | 预期效果 |
|--------|------|----------|
| **Canvas渲染** | requestAnimationFrame + 离屏Canvas | 60fps流畅 |
| **DOM操作** | 虚拟滚动 (vue-virtual-scroller) | 大列表无卡顿 |
| **图片加载** | 懒加载 + WebP格式 | 内存占用-40% |
| **动画** | CSS GPU加速 + will-change | 流畅度+30% |

### 5.2 网络性能

| 优化点 | 方案 | 预期效果 |
|--------|------|----------|
| **消息压缩** | Gzip/Brotli压缩 | 流量-60% |
| **心跳优化** | 自适应间隔 (10s-60s) | 连接稳定性↑ |
| **断线重连** | 指数退避 (1s, 2s, 4s...) | 重连成功率95%+ |
| **本地缓存** | IndexedDB + LRU淘汰 | 离线可用 |

### 5.3 构建优化

```javascript
// electron.vite.config.ts
export default defineConfig({
  build: {
    minify: 'esbuild',
    target: ['chrome118', 'node18'],
    rollupOptions: {
      output: {
        manualChunks: {
          vendor: ['vue', 'pinia', 'vue-router'],
          echarts: ['echarts'],
        },
      },
    },
  },
  // 代码分割 + Tree Shaking
});
```

---

## 6. 安全性考虑

### 6.1 进程隔离

```typescript
// preload.ts - 安全的IPC桥接
contextBridge.exposeInMainWorld('electronAPI', {
  // 白名单机制 - 只暴露必要API
  sendDanmaku: (data) => ipcRenderer.invoke('danmaku:send', data),
  connectServer: (config) => ipcRenderer.invoke('server:connect', config),
  
  // 禁止直接访问Node.js API
  // process, require, __dirname 等不可访问
});
```

### 6.2 数据安全

- **传输加密**: WSS/TLS
- **存储加密**: safeStorage (Electron内置)
- **XSS防护**: DOMPurify (用户输入过滤)
- **CSRF防护**: Token + SameSite Cookie

---

## 7. 开发环境配置

### 7.1 必需工具

```bash
# Node.js版本要求
node >= 18.0.0
npm >= 9.0.0

# 全局工具
npm install -g @electron-forge/cli
```

### 7.2 开发脚本

```json
{
  "scripts": {
    "dev": "electron-vite dev",
    "build": "electron-vite build",
    "preview": "electron-vite preview",
    "package": "electron-builder --win",
    "test": "vitest",
    "lint": "eslint . --ext .vue,.ts,.tsx",
    "typecheck": "vue-tsc --noEmit"
  }
}
```

### 7.3 IDE推荐

- **VS Code**: 配合Volar插件
- **WebStorm**: 原生Vue支持
- **推荐插件**:
  - Vue Language Features (Volar)
  - TypeScript Vue Plugin (Volar)
  - Tailwind CSS IntelliSense
  - ESLint
  - Prettier

---

## 8. 部署与发布

### 8.1 构建产物

```
dist/
├── win-unpacked/           # Windows绿色版
│   └── AI-Live-Room.exe
├── AI-Live-Room-Setup-1.0.0.exe  # Windows安装包
├── mac/                    # macOS版
│   └── AI-Live-Room.app
└── linux/                  # Linux版
    └── ai-live-room
```

### 8.2 自动更新

```typescript
// 使用electron-updater
import { autoUpdater } from 'electron-updater';

autoUpdater.setFeedURL({
  provider: 'github',
  owner: 'your-org',
  repo: 'ai-live-room',
});

autoUpdater.checkForUpdatesAndNotify();
```

---

## 9. 监控与日志

### 9.1 日志级别

```typescript
enum LogLevel {
  ERROR = 0,  // 错误日志 (必须记录)
  WARN  = 1,  // 警告日志
  INFO  = 2,  // 信息日志
  DEBUG = 3,  // 调试日志 (开发环境)
}

// 生产环境只记录ERROR和WARN
// 开发环境记录全部级别
```

### 9.2 性能监控

```typescript
// 使用performance API
const observer = new PerformanceObserver((list) => {
  for (const entry of list.getEntries()) {
    if (entry.duration > 100) {  // 超过100ms的操作
      logger.warn(`Slow operation: ${entry.name} took ${entry.duration}ms`);
    }
  }
});
observer.observe({ entryTypes: ['measure'] });
```

---

## 10. 后续扩展规划

### 10.1 插件系统
- 基于ESM的插件加载器
- 插件市场 (未来规划)

### 10.2 多端同步
- 移动端 (React Native / Flutter)
- Web端 (响应式适配)
- 小程序 (微信/抖音)

### 10.3 AI功能集成
- 智能弹幕过滤 (基于ML)
- AI主播数字人
- 实时翻译 (多语言弹幕)

---

**文档版本**: v1.0  
**最后更新**: 2026-05-17  
**技术负责人**: AI Assistant Team
