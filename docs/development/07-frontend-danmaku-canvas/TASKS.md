# 模块 07 — 前端弹幕 Canvas 渲染 开发任务

> **前置条件**: 模块 04 测试通过

---

## Task 7.1: 创建 DanmakuCanvas 组件

**文件**: `frontend/src/components/player/DanmakuCanvas.vue`

Canvas 弹幕渲染引擎，requestAnimationFrame 驱动

---

## Task 7.2: 替换 RoomView 弹幕层

**文件**: `frontend/src/views/RoomView.vue`

删除旧 CSS 弹幕叠加代码，替换为 `<DanmakuCanvas>`

---

## Task 7.3: 编译验证

```bash
npx vue-tsc --noEmit && npx vite build
```

---

*完成后运行 TESTS.md*
