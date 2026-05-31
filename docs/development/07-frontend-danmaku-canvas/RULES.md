# 模块 07 — 前端弹幕 Canvas 渲染 开发规则

> **前置模块**: 模块 04 必须全部测试通过

---

## 1. 模块定位

用 `<canvas>` 元素替代当前 RoomView 中基于 CSS `animate-slide-in` 的弹幕叠加层，实现高性能弹幕渲染。

---

## 2. 文件变更

| 操作 | 文件路径 |
|------|----------|
| **新建** | `frontend/src/components/player/DanmakuCanvas.vue` |
| **修改** | `frontend/src/views/RoomView.vue` |

---

## 3. DanmakuCanvas 设计

### Props
```typescript
{ danmakuList: DanmakuMessage[] }
```

### 实现要点
1. `onMounted`: 获取 canvas context，启动 `requestAnimationFrame` 循环
2. 每个弹幕分配随机 y 轴轨道（行高 32px，共 8 轨道）
3. 每帧更新 x 坐标（速度统一 ~2px/frame）
4. 弹幕移出左侧边界后从渲染列表移除
5. `watch` danmakuList 新消息时加入渲染队列
6. 字体大小 16px，`fontWeight: 500`
7. 文字颜色使用 message.color
8. 文字阴影 `shadowBlur: 4, shadowColor: 'currentColor'`

### Canvas 特效
- 半透明黑色背景栏（1行高）：`fillStyle: 'rgba(0,0,0,0.4)'`

---

## 4. RoomView 集成

将现有的弹幕叠加层（CSS 动画 div）替换为：
```vue
<DanmakuCanvas :danmakuList="roomStore.danmakuList" />
```

---

## 5. 关键约束

- ⚠️ 删除旧的 CSS 弹幕动画代码 (animate-slide-in)
- ⚠️ Canvas 尺寸响应式（ResizeObserver 监听容器大小）
- ⚠️ `onUnmounted` 取消 `requestAnimationFrame`
- ⚠️ Canvas 在视频区内部（绝对定位在 video 上方）

---

*遵循以上规则和全局主规则进行开发。*
