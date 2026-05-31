# 模块 08 — 前端设置面板 + 主题系统 开发任务

> **前置条件**: 模块 07 测试通过

---

## Task 8.1: 创建 useTheme composable

**文件**: `frontend/src/composables/useTheme.ts`

Theme 枚举 + CSS 变量切换 + localStorage 持久化 + ref + apply 逻辑

## Task 8.1b: 更新现有页面使用 CSS 变量

**文件**: `LoginView.vue`, `RoomView.vue`

将硬编码的背景色（如 `bg-[#0F0F23]`）替换为 `style="background-color: var(--bg-primary)"` 等 CSS 变量引用

---

## Task 8.2: 创建 SettingsView 页面

**文件**: `frontend/src/views/SettingsView.vue`

包含：主题切换区域 + 弹幕设置（密度/速度/透明度/字体大小） + 登出按钮

---

## Task 8.3: 追加路由

**文件**: `frontend/src/router/index.ts`

增加 `/settings` 路由

---

## Task 8.4: Settings 按钮跳转

**文件**: `frontend/src/views/RoomView.vue`

底部面板 Settings 按钮 `@click` → `router.push('/settings')`

---

## Task 8.5: 编译验证

```bash
npx vue-tsc --noEmit && npx vite build
```

---

*完成后运行 TESTS.md*
