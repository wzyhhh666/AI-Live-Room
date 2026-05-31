# 模块 09 — Electron 打包分发 开发规则

> **前置模块**: 模块 08 必须全部测试通过

---

## 1. 模块定位

创建 Electron 主进程代码和打包配置，将 Vue 3 前端打包为 Windows 桌面客户端。

---

## 2. 文件变更

| 操作 | 文件路径 |
|------|----------|
| **新建** | `frontend/electron/main.ts` |
| **新建** | `frontend/electron/preload.ts` |
| **新建** | `frontend/electron-builder.yml` |
| **修改** | `frontend/package.json` |

---

## 3. Electron 主进程设计

### main.ts

```typescript
import { app, BrowserWindow, shell } from 'electron'
import path from 'path'

let mainWindow: BrowserWindow | null = null

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1280,
    height: 800,
    minWidth: 1024,
    minHeight: 600,
    frame: false,              // 无边框（自定义标题栏）
    transparent: false,
    backgroundColor: '#0F0F23',
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      nodeIntegration: false,
      contextIsolation: true,
    }
  })
  
  if (process.env.VITE_DEV_SERVER_URL) {
    mainWindow.loadURL(process.env.VITE_DEV_SERVER_URL)
    mainWindow.webContents.openDevTools()
  } else {
    mainWindow.loadFile(path.join(__dirname, '../dist/index.html'))
  }
}

app.whenReady().then(createWindow)
app.on('window-all-closed', () => { if (process.platform !== 'darwin') app.quit() })
```

### preload.ts

```typescript
import { contextBridge, ipcRenderer } from 'electron'

contextBridge.exposeInMainWorld('electronAPI', {
  windowControl: {
    minimize: () => ipcRenderer.invoke('window:minimize'),
    maximize: () => ipcRenderer.invoke('window:maximize'),
    close: () => ipcRenderer.invoke('window:close'),
  }
})
```

---

## 4. 打包配置 (electron-builder.yml)

```yaml
appId: com.ai-live-room.desktop
productName: AI Live Room
directories:
  output: release
win:
  target: nsis
  icon: resources/icon.ico
nsis:
  oneClick: false
  allowToChangeInstallationDirectory: true
files:
  - dist/**/*
  - electron/**/*
```

---

## 5. package.json 修改

追加 scripts：
```json
{
  "main": "electron/main.js",
  "scripts": {
    "dev:electron": "electron-vite dev",
    "build:electron": "electron-vite build && electron-builder --win",
    "build:win": "electron-builder --win"
  }
}
```

追加 devDependencies：
```json
{
  "electron": "^28.0.0",
  "electron-builder": "^24.9.0",
  "electron-vite": "^2.0.0"
}
```

---

## 6. 关键约束

- ⚠️ `contextIsolation: true`（安全）
- ⚠️ `nodeIntegration: false`（安全）
- ⚠️ 仅暴露 `windowControl` 三个方法
- ⚠️ 先只构建 Windows 版本
- ⚠️ 开发模式下可访问 `VITE_DEV_SERVER_URL`

---

*遵循以上规则和全局主规则进行开发。*
