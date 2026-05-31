# 模块 02 — 前端礼物系统 开发规则

> **前置模块**: 模块 01（后端礼物系统）必须通过全部测试
> **前置阅读**: [全局主规则](../00-master-rules/MASTER_RULES.md)

---

## 1. 模块定位

在现有 RoomView.vue 右侧面板中，将礼物按钮从骨架状态变为功能完整的礼物系统，包含礼物选择面板、发送、特效动画和排行榜。

---

## 2. 文件变更清单

| 操作 | 文件路径 | 说明 |
|------|----------|------|
| **新建** | `frontend/src/stores/gift.ts` | 礼物 Pinia Store |
| **新建** | `frontend/src/components/gift/GiftPanel.vue` | 礼物选择面板 |
| **新建** | `frontend/src/components/gift/GiftItem.vue` | 单个礼物卡片 |
| **新建** | `frontend/src/components/gift/GiftEffect.vue` | 全屏礼物特效动画 |
| **新建** | `frontend/src/components/gift/GiftRank.vue` | 礼物贡献排行榜 |
| **修改** | `frontend/src/services/api.ts` | 追加 3 个 HTTP/Socket 方法 |
| **修改** | `frontend/src/views/RoomView.vue` | 集成礼物面板、特效、排行榜 |

---

## 3. Store 设计

### 3.1 `gift.ts` 接口定义

```typescript
export interface GiftItem {
  id: number
  giftName: string
  giftIcon: string
  price: number
  effectType: 'normal' | 'explosion' | 'rain' | 'rocket'
  sortOrder: number
}

export interface GiftRankEntry {
  senderId: number
  senderName: string
  totalAmount: number
}

export interface GiftEvent {
  id: number
  giftName: string
  giftIcon: string
  count: number
  totalPrice: number
  senderName: string
  effectType: string
  timestamp: number
}
```

### 3.2 Store 状态

```typescript
{
  giftList: GiftItem[],        // 所有可用礼物
  selectedGift: GiftItem | null, // 当前选中礼物
  sendCount: number,           // 发送数量（默认1）
  giftRank: GiftRankEntry[],   // 排行榜数据
  lastGift: GiftEvent | null,  // 最近收到的礼物事件（驱动特效）
}
```

### 3.3 Store 方法

- `fetchGiftList()` — 调用 `apiService.getGiftList()` 获取礼物列表
- `selectGift(gift)` — 选中礼物
- `setCount(n)` — 设置发送数量
- `sendGift(roomId)` — 通过 Socket 发送（乐观更新）
- `updateRank(entries)` — 更新排行榜
- `triggerEffect(event)` — 触发特效

---

## 4. 组件拆分

### 4.1 GiftItem.vue — 礼物卡片

```
 ┌──────────────┐
 │   🚀         │  ← giftIcon emoji
 │  Rocket      │  ← giftName
 │  $100.00     │  ← price
 └──────────────┘
```

- Props: `gift: GiftItem`, `selected: boolean`, `count: number`
- Emit: `@click` — 选中该礼物
- 选中状态：`ring-2 ring-pink-400 bg-pink-500/10`
- 未选中：`border border-gray-700`

### 4.2 GiftPanel.vue — 礼物选择面板

```
 ┌─────────────────────────────┐
 │  🎁  Send Gift          [X] │  ← 标题 + 关闭按钮
 ├─────────────────────────────┤
 │  ┌─────┐ ┌─────┐ ┌─────┐  │
 │  │🪄   │ │👍   │ │🌸   │  │  ← 3×2 网格
 │  │$1   │ │$2   │ │$5   │  │
 │  └─────┘ └─────┘ └─────┘  │
 │  ┌─────┐ ┌─────┐ ┌─────┐  │
 │  │🏎️   │ │🚀   │ │🎪   │  │
 │  │$50  │ │$100 │ │$500 │  │
 │  └─────┘ └─────┘ └─────┘  │
 ├─────────────────────────────┤
 │  ×1  ×5  ×10  ×66        │  ← 数量选择器
 │  [    Send Gift    ]       │  ← 发送按钮
 └─────────────────────────────┘
```

- Props: `show: boolean`, `roomId: string`
- Emit: `@close`
- 使用 `GiftItem` 子组件
- 底部数量选择器和发送按钮
- 使用 `useGiftStore()` 获取状态

### 4.3 GiftEffect.vue — 特效层

固定在 RoomView 的弹幕叠加层上方（z-index 更高）。

- Props: `effect: GiftEvent | null`
- Watch `effect` 变化时触发对应动画：
  - `normal`: 右下角飘入 + 3s 后消失
  - `explosion`: 全屏粒子爆炸 + 文字绽放
  - `rain`: 从顶部倾泻对应 emoji 粒子
  - `rocket`: 底部发射飞向顶部
- 使用 CSS `@keyframes` 动画，不引入额外库

### 4.4 GiftRank.vue — 排行榜

```
 ┌─────────────────────────┐
 │  🏆 Gift Rankings       │
 ├─────────────────────────┤
 │  🥇 UserA    $1,500    │
 │  🥈 UserB    $800      │
 │  🥉 UserC    $500      │
 │  4  UserD    $300      │
 │  ...                    │
 └─────────────────────────┘
```

- 显示 Top 20
- 前三名金银铜牌 emoji
- Props: `rankings: GiftRankEntry[]`
- 监听 `gift-rank-update` Socket 事件自动更新

---

## 5. RoomView 集成

修改 `RoomView.vue`，在右侧面板中：

1. **Tab 切换**: 礼物按钮点击后切换右侧面板内容（聊天 / 礼物 / 排行），不复用单个 panel
2. **Socket 监听**: 在 `onMounted` 中追加：
   - `new-gift` → `giftStore.triggerEffect(data)`
   - `gift-rank-update` → `giftStore.updateRank(data)`
3. **发送礼物后的乐观渲染**: 前端本地立即展示发送结果

**关键集成点（RoomView.vue 现有代码位置）**:
- 右侧面板 header 中的 `Gift` 按钮（约第323行）：改为切换面板模式
- 右侧面板 body（约第332行）：根据模式显示 `ChatPanel` / `GiftPanel` / `GiftRank`
- 弹幕叠加层上方：放置 `<GiftEffect>` 组件
- `onMounted` 中的 Socket 监听（约第115行之后）：追加礼物事件

---

## 6. API Service 追加

在 `frontend/src/services/api.ts` 的 `ApiService` 类中追加方法：

```typescript
async getGiftList()
async sendGiftHttp(roomId, giftId, count, userId, username)
async getGiftRank(roomId, limit = 20)
```

---

## 7. 样式规则

- 面板背景: `bg-gray-900/80 backdrop-blur-sm`
- 边框: `border-gray-800/50`
- 选中高亮: `ring-2 ring-pink-400 bg-pink-500/10`
- 按钮渐变: `bg-gradient-to-r from-pink-500 to-purple-500`
- 排行榜前三: 金色 `text-yellow-400` / 银色 `text-gray-300` / 铜色 `text-amber-600`

---

## 8. 关键约束

- ⚠️ **不引入新 npm 包**：特效用纯 CSS 动画
- ⚠️ **面板复用 RoomView 右侧区域**：不创建新 route/页面
- ⚠️ **SendGift 使用 WebSocket**（与 send-danmaku 一致模式），不用 HTTP POST
- ⚠️ **特效动画 ≤3s**：不阻塞后续礼物动画

---

*遵循以上规则和全局主规则进行开发。*
