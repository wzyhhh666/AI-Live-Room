# 模块 09 — Electron 打包分发 开发任务

> **前置条件**: 模块 08 全部测试通过

---

## Task 9.1: 安装 Electron 依赖

```bash
cd frontend
npm install --save-dev electron electron-builder electron-vite
```

---

## Task 9.2: 创建 electron/main.ts

**文件**: `frontend/electron/main.ts`

BrowserWindow 创建逻辑、Vite dev server 代理、窗口控制 IPC

---

## Task 9.3: 创建 electron/preload.ts

**文件**: `frontend/electron/preload.ts`

`contextBridge.exposeInMainWorld('electronAPI', { windowControl })`

---

## Task 9.4: 创建 electron-builder.yml

**文件**: `frontend/electron-builder.yml`

打包配置，输出到 `release/` 目录

---

## Task 9.5: 更新 package.json

**文件**: `frontend/package.json`

追加 `main`、`scripts`、`devDependencies`

---

## Task 9.6: 编译验证

```bash
npm install
npx vite build
npx electron-builder --win --dir   # 仅打包不解压安装包，快速验证
```

---

*完成后运行 TESTS.md*
