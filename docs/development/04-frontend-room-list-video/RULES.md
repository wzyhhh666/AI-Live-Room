# 模块 04 — 前端房间列表 + 视频播放器 开发规则

> **前置模块**: 模块 02 + 模块 03 必须全部测试通过
> **前置阅读**: [全局主规则](../00-master-rules/MASTER_RULES.md)

---

## 1. 模块定位

创建房间列表页面（RoomListView）作为登录后的主页，用 RoomCard 组件展示可用房间。创建 VideoPlayer 组件替换现有的视频区占位符。

---

## 2. 文件变更清单

| 操作 | 文件路径 | 说明 |
|------|----------|------|
| **新建** | `frontend/src/views/RoomListView.vue` | 房间列表页 |
| **新建** | `frontend/src/components/room/RoomCard.vue` | 房间卡片 |
| **新建** | `frontend/src/components/player/VideoPlayer.vue` | 视频播放器 |
| **修改** | `frontend/src/router/index.ts` | 增加 /rooms 路由 + 登录 redirect |
| **修改** | `frontend/src/views/RoomView.vue` | 替换视频区占位符 |
| **修改** | `frontend/src/services/api.ts` | 追加 getRoomList / createRoom |

---

## 3. RoomListView 页面设计

### 布局

```
┌──────────────────────────────────────────────┐
│  🤖 AI LIVE    + Create Room    👤 User      │  ← 页面头部
├──────────────────────────────────────────────┤
│  ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐│
│  │RoomCard│ │RoomCard│ │RoomCard│ │RoomCard││  ← 4列网格
│  └────────┘ └────────┘ └────────┘ └────────┘│
│  ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐│
│  │RoomCard│ │RoomCard│ │RoomCard│ │RoomCard││
│  └────────┘ └────────┘ └────────┘ └────────┘│
├──────────────────────────────────────────────┤
│           ← 1 2 3 ... → (分页)              │
└──────────────────────────────────────────────┘
```

### 关键代码结构

```vue
<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { useUserStore } from '@/stores/user'
import apiService from '@/services/api'

const router = useRouter()
const userStore = useUserStore()

const rooms = ref([])
const loading = ref(true)

onMounted(async () => {
  const result = await apiService.getRoomList()
  if (result.success) rooms.value = result.data.rooms
  loading.value = false
})

function goToRoom(roomId: number) {
  router.push(`/room/${roomId}`)
}
</script>
```

---

## 4. RoomCard 组件接口

```typescript
interface Props {
  roomId: number
  roomName: string
  hostName: string
  hostAvatar?: string      // 未提供时生成: `https://api.dicebear.com/7.x/avataaars/svg?seed=${hostName}`
  title?: string           // 直播间标题，显示为副标题
  onlineCount: number
  coverImage?: string
  state: 'idle' | 'living' | 'closed'
}

// Emit: @click → 导航到 /room/:roomId
```

- 头像默认使用 DiceBear API 生成: `https://api.dicebear.com/7.x/avataaars/svg?seed=${encodeURIComponent(hostName)}`

---

## 5. VideoPlayer 组件

- 使用 `<video>` 原生标签
- 默认 HLS 测试流: `https://test-streams.mux.dev/x36xhzz/x36xhzz.m3u8`
- 无视频时显示赛博朋克网格 + 霓虹动画背景（不能是纯黑/纯色块）
- 播放/暂停按钮
- 组件无 props（自包含）

---

## 6. 路由变更

```typescript
// router/index.ts 修改前后对比
routes: [
  { path: '/', name: 'login', component: LoginView, meta: { requiresAuth: false } },
  { path: '/rooms', name: 'roomList', component: RoomListView, meta: { requiresAuth: true } }, // 新增
  { path: '/room/:roomId?', name: 'room', component: RoomView, meta: { requiresAuth: true } }, // 保留
]

// 导航守卫中
if (to.name === 'login' && userStore.isLoggedIn) {
  next({ name: 'roomList' }) // 改为跳转到房间列表，不是直接进房
}
```

---

## 7. 样式规则

- 页面背景: `bg-[#0F0F23]`
- 卡片背景: `bg-gray-900/80 backdrop-blur-sm border border-gray-800/50`
- 卡片悬停: `hover:scale-105 hover:border-cyan-400/50 hover:shadow-lg hover:shadow-cyan-500/20 transition-all`
- LIVE 标签: `bg-red-600/90 text-white px-2 py-0.5 rounded-full text-xs animate-pulse`
- 创建房间按钮: `bg-gradient-to-r from-cyan-500 to-blue-500`

---

## 8. 关键约束

- ⚠️ **RoomView 不改变其核心功能**：只替换视频区的占位符
- ⚠️ **登录成功后跳转到 `/rooms`**，不是直接进房间
- ⚠️ **VideoPlayer 在无视频源时不能是空白**：必须有赛博朋克装饰
- ⚠️ **不引入 video.js 或 hls.js**：使用原生 `<video>` 标签

---

*遵循以上规则和全局主规则进行开发。*
