# 模块 08 — 前端设置面板 + 主题系统 开发规则

> **前置模块**: 模块 07 必须全部测试通过

---

## 1. 模块定位

创建设置页面 + 主题系统 composable，支持暗色/亮色/护眼三种主题切换，包含弹幕偏好设置和登出功能。

---

## 2. 文件变更

| 操作 | 文件路径 |
|------|----------|
| **新建** | `frontend/src/composables/useTheme.ts` |
| **新建** | `frontend/src/views/SettingsView.vue` |
| **修改** | `frontend/src/router/index.ts` |
| **修改** | `frontend/src/views/RoomView.vue` |

---

## 3. useTheme Composable

```typescript
type Theme = 'dark' | 'light' | 'warm'  // 暗色=赛博朋克 / 亮色 / 护眼

// 返回: { theme, setTheme, isDark, isLight, isWarm }
```

- 默认 `dark`
- 切换时修改 `document.documentElement` CSS 变量
- `localStorage` 持久化

### CSS 变量映射

| 变量 | dark | light | warm |
|------|------|-------|------|
| `--bg-primary` | `#0F0F23` | `#F8FAFC` | `#1A3320` |
| `--bg-secondary` | `#1a1a2e` | `#FFFFFF` | `#2A4A30` |
| `--text-primary` | `#FFFFFF` | `#1E293B` | `#E8F5E9` |
| `--text-secondary` | `#9CA3AF` | `#64748B` | `#A5D6A7` |
| `--border-color` | `#374151` | `#E2E8F0` | `#388E3C` |
| `--accent-pink` | `#FF006E` | `#DB2777` | `#E91E63` |
| `--accent-purple` | `#8338EC` | `#7C3AED` | `#9C27B0` |
| `--accent-cyan` | `#00D4FF` | `#0891B2` | `#00BCD4` |

---

## 4. SettingsView 页面设计

```
┌─────────────────────────────────────────────────┐
│  ⚙ Settings                                    │
├─────────────────────────────────────────────────┤
│  Theme                                         │
│  [ Dark ] [ Light ] [ Warm ]                   │
├─────────────────────────────────────────────────┤
│  Danmaku Settings                              │
│  Density:  ───●──────── (50%)                  │
│  Speed:    ──────●───── (60%)                  │
│  Opacity:  ────●─────── (40%)                  │
│  Font Size: ──●───────── (Small/Normal/Large)  │
├─────────────────────────────────────────────────┤
│  Account                                       │
│  [ ❌ Logout ]                                  │
├─────────────────────────────────────────────────┤
│  v1.0.0 Cyberpunk Edition                     │
└─────────────────────────────────────────────────┘
```

---

## 5. 关键约束

- ⚠️ 全局 CSS 变量统一通过 `document.documentElement.style.setProperty()` 设置
- ⚠️ 弹幕设置面板当前阶段只做 UI 骨架（不实际连接控制逻辑）
- ⚠️ 登出调用 `userStore.logout()` + `router.push('/')`
- ⚠️ `main.ts` 中启动时调用 `initFromStorage()` 恢复主题偏好
- ⚠️ **关键**: `LoginView.vue` 和 `RoomView.vue` 等现有页面中的硬编码背景色（如 `bg-[#0F0F23]`）需同步替换为 CSS 变量引用（如 `style="background-color: var(--bg-primary)"`），否则主题切换不会生效

---

*遵循以上规则和全局主规则进行开发。*
