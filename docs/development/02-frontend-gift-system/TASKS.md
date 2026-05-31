# 模块 02 — 前端礼物系统 开发任务

> **前置条件**: 模块 01（后端礼物系统）全部测试通过
> **前置阅读**: [RULES.md](./RULES.md) | [全局主规则](../00-master-rules/MASTER_RULES.md)

---

## 任务执行顺序（严格按序号执行）

---

### Task 2.1: 创建 Gift Store

**文件**: `frontend/src/stores/gift.ts`（新建）

**实现**:
1. 定义 `GiftItem`、`GiftRankEntry`、`GiftEvent` 接口
2. 创建 `useGiftStore`，使用 Composition API 风格
3. 实现状态：`giftList`、`selectedGift`、`sendCount`、`giftRank`、`lastGift`
4. 实现方法：`fetchGiftList`、`selectGift`、`setCount`、`sendGift`、`updateRank`、`triggerEffect`

**参考文件**: `stores/room.ts`（同风格）

---

### Task 2.2: 追加 API Service 方法

**文件**: `frontend/src/services/api.ts`

**操作**: 在 `ApiService` 类中追加 `getGiftList()`、`sendGiftHttp()`、`getGiftRank()` 方法

---

### Task 2.3: 创建 GiftItem 组件

**文件**: `frontend/src/components/gift/GiftItem.vue`（新建）

**实现**: 单个礼物卡片，展示 emoji + 名称 + 价格，选中态高亮

---

### Task 2.4: 创建 GiftPanel 组件

**文件**: `frontend/src/components/gift/GiftPanel.vue`（新建）

**实现**:
1. 3×2 礼物网格（`GiftItem` 子组件）
2. 数量选择器（×1 / ×5 / ×10 / ×66）
3. 发送按钮
4. 总价显示
5. 关闭按钮（`@close` emit）

---

### Task 2.5: 创建 GiftEffect 组件

**文件**: `frontend/src/components/gift/GiftEffect.vue`（新建）

**实现**:
1. Watch `lastGift` 变化
2. 根据 `effectType` 播放不同 CSS 动画
3. 动画播放完自动清理

---

### Task 2.6: 创建 GiftRank 组件

**文件**: `frontend/src/components/gift/GiftRank.vue`（新建）

**实现**:
1. 展示 Top 20 排行榜
2. 前三名特殊样式（金银铜）
3. 监听 Store 的 `giftRank` 变化

---

### Task 2.7: 集成到 RoomView.vue

**文件**: `frontend/src/views/RoomView.vue`

**操作**:
1. 导入所有新组件和 `useGiftStore`
2. 礼物按钮改为切换面板模式（`showGiftPanel` / `showGiftRank`）
3. 右侧面板内容根据模式显示不同组件
4. 在弹幕叠加层上方添加 `<GiftEffect>` 组件
5. `onMounted` 中追加 `new-gift`、`gift-rank-update` Socket 事件监听
6. `onMounted` 中调用 `giftStore.fetchGiftList()` 加载礼物列表

---

### Task 2.8: TypeScript 编译 + 构建验证

```bash
cd frontend
npx vue-tsc --noEmit
npx vite build
```

确保零错误。

---

*所有任务完成后，运行 TESTS.md 中的全部测试用例。*
