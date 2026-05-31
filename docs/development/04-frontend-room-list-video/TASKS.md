# 模块 04 — 前端房间列表 + 视频播放器 开发任务

> **前置条件**: 模块 03 全部测试通过
> **前置阅读**: [RULES.md](./RULES.md)

---

## 任务执行顺序

---

### Task 4.1: 追加 API Service 方法

**文件**: `frontend/src/services/api.ts`

在 `ApiService` 类中追加：
1. `getRoomList(page, pageSize)` — `GET /api/rooms`
2. `createRoom(data)` — `POST /api/rooms`

---

### Task 4.2: 创建 RoomListView 页面

**文件**: `frontend/src/views/RoomListView.vue`（新建）

**内容**:
1. 顶部区域：AI LIVE 标题 + 创建房间按钮
2. 网格区域：`grid grid-cols-4 gap-4` 布局
3. 从 API 加载房间列表
4. 赛博朋克风格底色

---

### Task 4.3: 创建 RoomCard 组件

**文件**: `frontend/src/components/room/RoomCard.vue`（新建）

**内容**:
1. 封面图片
2. 房间名称
3. 主播头像 + 昵称
4. 在线人数 + LIVE 脉冲标签
5. `@click` → `router.push('/room/' + roomId)`
6. 悬停效果：`scale-105` + 发光边框

---

### Task 4.4: 创建 VideoPlayer 组件

**文件**: `frontend/src/components/player/VideoPlayer.vue`（新建）

**内容**:
1. 替换 RoomView 中视频区的占位符
2. 用 `<video>` 元素 + 赛博朋克风格装饰
3. 当前阶段可用 demo 视频流（比如 Big Buck Bunny 的 HLS 测试流）
4. 作为备选，无视频源时显示赛博朋克装饰背景（不能是纯黑屏）

---

### Task 4.5: 修改路由

**文件**: `frontend/src/router/index.ts`

**操作**:
1. 增加 `/rooms` 路由，指向 `RoomListView`
2. `meta: { requiresAuth: true }`
3. 导航守卫中，登录后跳转到 `/rooms` 而不是直接 `/room/1`

---

### Task 4.6: 修改 RoomView 集成 VideoPlayer

**文件**: `frontend/src/views/RoomView.vue`

将视频区中央的 `<VideoOff>` 占位符替换为 `<VideoPlayer>` 组件

---

### Task 4.7: 编译验证

```bash
npx vue-tsc --noEmit && npx vite build
```

---

*所有任务完成后，运行 TESTS.md 中的全部测试用例。*
