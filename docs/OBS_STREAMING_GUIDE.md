# OBS Studio & 浏览器推流指南

---

## 方式一：浏览器推流 ✅（推荐，无需安装软件）

主播不需要安装任何软件，打开浏览器直接推流：

1. 登录前端 → 进入直播间
2. 点击顶部 **「开始直播」** 按钮
3. 浏览器弹出**摄像头/麦克风权限请求** → 点击允许
4. 在主播控制台点击 **「开始推流」**
5. 画面预览显示后，推流即开始
6. 观众刷新直播间页面即可观看（FLV 1-3秒延迟）

> 推流地址：`http://localhost:5173/studio/{roomId}`

---

## 方式二：OBS 推流（专业推流，画质更好）

## 前提条件

- **OBS Studio** 已安装 ([https://obsproject.com/](https://obsproject.com/))
- 所有 Docker 容器正常运行：`docker ps`
- 直播间房间已创建（登录前端创建）

---

## 第一步：获取推流地址和密钥

### 方式 A：通过 API（开发环境）

```bash
curl -X POST http://localhost:3000/api/v1/rooms/{roomId}/stream/start
```

返回示例：
```json
{
  "success": true,
  "data": {
    "streamKey": "room_1_a1b2c3d4_662a1b2c",
    "rtmpUrl": "rtmp://localhost:1935/live/room_1_a1b2c3d4_662a1b2c",
    "hlsUrl": "http://localhost:8088/hls/room_1_a1b2c3d4_662a1b2c.m3u8",
    "flvUrl": "http://localhost:8080/live/room_1_a1b2c3d4_662a1b2c.flv"
  }
}
```

### 方式 B：直接使用固定地址（测试用）

```
推流地址: rtmp://localhost:1935/live
推流密钥: room_{你的房间ID}_test
```

---

## 第二步：配置 OBS

### 1. 打开 OBS Studio → 设置 → 推流

| 字段 | 值 |
|------|-----|
| 服务 | **自定义...** |
| 服务器 | `rtmp://localhost:1935/live` |
| 推流密钥 | `room_1_a1b2c3d4_662a1b2c`（使用 API 返回的密钥） |

### 2. 设置 → 输出（推荐参数）

| 字段 | 值 |
|------|-----|
| 输出模式 | **高级** |
| 编码器 | **硬件编码器**（H.264/AVC） |
| 码率控制 | **CBR** |
| 比特率 | **2500 Kbps**（1080p）或 **1000 Kbps**（720p） |
| 关键帧间隔 | **2 秒** |
| 预设 | **质量优先** |

### 3. 设置 → 视频（推荐参数）

| 字段 | 值 |
|------|-----|
| 基础分辨率 | `1920x1080` 或 `1280x720` |
| 输出分辨率 | 同上（或 `1280x720` 推流更流畅） |
| 帧率 | `30` |
| 缩放滤镜 | **Lanczos** |

### 4. 添加素材源

点击「+」号添加：
- **显示器采集** — 捕获屏幕
- **视频采集设备** — 连接摄像头
- **音频输入采集** — 麦克风
- **音频输出采集** — 系统声音

---

## 第三步：开始推流

1. 检查 OBS 右下角状态 → 显示「绿色信号正常」
2. 点击 **「开始推流」** 按钮
3. 直播间页面自动检测到 `stream-started` 事件
4. 观众端使用 **FLV 播放**（延迟 1-3 秒）或 **HLS 播放**（延迟 8-15 秒）

### 验证推流成功

```bash
# 查看 SRS 日志确认推流连接
docker logs chatroom-srs --tail 20

# 如果配置了回调，服务端日志会显示
curl http://localhost:3000/api/v1/rooms/1/stream/info
# → isLiving: true
```

---

## 前端访问推流地址

推流成功后，前端通过 WebSocket 事件 `stream-started` 自动接收推流地址：

```
FLV:  http://localhost:8080/live/room_{roomId}.flv    ← 低延迟 1-3s
HLS:  http://localhost:8088/hls/room_{roomId}.m3u8    ← 兼容性更好 8-15s
```

VideoPlayer 会自动检测 URL 后缀：
- `.flv` → 使用 **mpegts.js** 播放（1-3 秒延迟）
- `.m3u8` → 使用浏览器原生 **HLS** 播放（8-15 秒延迟）

---

## 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| OBS 连接失败 | SRS 未启动 | `docker start chatroom-srs` |
| 推流成功但画面黑屏 | 浏览器安全策略 | 确保页面通过 HTTPS 或 localhost 访问 |
| 视频卡顿 | 码率过高 | 降低 OBS 比特率到 1500 Kbps |
| 音频不同步 | 帧率不匹配 | OBS 设置为 30fps，关键帧间隔 2s |
| 延迟太大 | 使用 HLS 播放 | 改用 FLV 播放（延迟 1-3s） |
