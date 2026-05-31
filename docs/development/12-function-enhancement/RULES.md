# 模块 12 — Phase 3：功能完善

> **前置阅读**: [架构设计 §12.4](../../ARCHITECTURE_DESIGN.md#124-phase-3-功能完善-p1) | [全局主规则](../00-master-rules/RULES.md)

---

## 1. 模块定位

Phase 3 在 Phase 1（流媒体层）+ Phase 2（C++ 实时层）搭建完成后，面向用户体验和安全性进行功能补齐。

**核心变更**：
- 6 个表情按钮从装饰变为可发送（复用弹幕通道）
- 表情弹幕渲染为大号字体 + 弹跳动画
- 敏感词库从 3 个占位词扩充到 1000+ 真实词条
- 新增 `sensitive_log` 审计日志表
- 推流密钥生成/验证/过期 API
- 房间统计 API

---

## 2. 表情功能规范

### 2.1 Emoji 列表

| emoji | name | type | 说明 |
|-------|------|------|------|
| 😀 | happy | face | 开心表情 |
| ❤️ | heart | heart | 爱心 |
| 👍 | like | face | 点赞 |
| 🎉 | celebrate | celebration | 庆祝 |
| 🔥 | fire | fire | 火热 |
| ✨ | sparkle | sparkle | 闪耀 |

### 2.2 消息格式

表情作为弹幕的一种特殊类型发送，复用 `send-danmaku` WebSocket 事件：

```json
// 前端 → Node.js (socket.js)
{
  "roomId": 1,
  "content": "😀",
  "color": "#ffdd00",
  "type": "emoji"
}
```

Node.js socket.js 收到后 PUBLISH 到 `danmaku:input`，C++ 处理并 PUBLISH 到 `danmaku:output`。

### 2.3 前端渲染规则

- emoji 类型弹幕在 DanmakuCanvas 中使用 **40px 大号字体**渲染
- 应用 **弹跳动画**（`bounce` easing）
- 表情弹幕 **不参与敏感词过滤**（前端发送时 type=emoji 标记）

---

## 3. 敏感词系统规范

### 3.1 sensitive_log 表

```sql
CREATE TABLE IF NOT EXISTS `sensitive_log` (
    `id` BIGINT AUTO_INCREMENT PRIMARY KEY,
    `room_id` INT NOT NULL,
    `user_id` INT NULL,
    `original_content` TEXT NOT NULL,
    `filtered_content` TEXT,
    `matched_words` VARCHAR(500),
    `action` ENUM('mask', 'block') NOT NULL,
    `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX `idx_created` (`created_at`),
    FOREIGN KEY (`room_id`) REFERENCES `rooms`(`id`) ON DELETE CASCADE,
    FOREIGN KEY (`user_id`) REFERENCES `users`(`id`) ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
```

### 3.2 敏感词来源

- **Node.js 侧**: `api-server/config/sensitive_words.json`（1000+ 词条）
- **C++ 侧**: `backend-server/config/sensitive_words.txt`（与 JSON 内容一致，按行分割）
- 词条来源：中文互联网常见敏感词（政治、暴力、色情、广告等分类）

### 3.3 敏感词审计流程

```
前端发送 → socket.js 限流检查
  → PUBLISH danmaku:input → C++ FilterService 过滤
    → 如果被过滤: PUBLISH danmaku:blocked + MySQL sensitive_log 插入
    → 如果通过: PUBLISH danmaku:output
```

---

## 4. 推流密钥规范

### 4.1 API 端点

| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| POST | `/api/v1/rooms/{roomId}/stream/start` | 生成推流密钥并启动推流记录 | 主播 Token |
| POST | `/api/v1/rooms/{roomId}/stream/stop` | 停止推流并更新记录 | 主播 Token |
| GET | `/api/v1/rooms/{roomId}/stream/info` | 获取推流地址和状态 | 无 |

### 4.2 密钥生成规则

- 格式: `{roomId}_{uuid_short}_{timestamp_hex}`
- 示例: `1_a1b2c3d4_662a1b2c`
- 存储到 `stream_records` 表
- 密钥有效期：推流期间有效，停推后过期

---

## 5. 房间统计 API

### 5.1 端点

| 方法 | 路径 | 说明 | 认证 |
|------|------|------|------|
| GET | `/api/v1/rooms/{roomId}/stats` | 房间统计信息 | 无 |

### 5.2 返回格式

```json
{
  "room_id": 1,
  "total_danmaku": 1234,
  "total_gifts": 56,
  "total_gift_value": "789.00",
  "peak_online": 89,
  "stream_duration_sec": 3600
}
```

---

## 6. 文件变更清单

| 操作 | 文件路径 | 说明 |
|------|----------|------|
| **修改** | `frontend/src/views/RoomView.vue` | 表情按钮 + @click |
| **修改** | `frontend/src/components/player/DanmakuCanvas.vue` | emoji 渲染增强 |
| **新增** | `api-server/src/services/sensitiveFilter.js` | 改造：加审计日志记录 |
| **修改** | `api-server/src/services/database.js` | 加 sensitive_log 建表 |
| **修改** | `api-server/config/sensitive_words.json` | 1000+ 词条 |
| **修改** | `backend-server/config/sensitive_words.txt` | 同步 1000+ 词条 |
| **修改** | `api-server/src/routes/stream.js` | 推流密钥 API |
| **新增** | `api-server/src/routes/stats.js` | 房间统计路由 |
| **修改** | `api-server/src/server.js` | 挂载 stats 路由 |

---

## 7. 测试验证

- ✅ 6 个表情按钮点击后发送 emoji 弹幕
- ✅ emoji 弹幕在 Canvas 上渲染为大号字体 + 弹跳动画
- ✅ sensitive_log 表存在并能正常写入
- ✅ 敏感词过滤正常（mask/block 两种动作）
- ✅ 推流密钥生成→验证→过期全流程
- ✅ 房间统计 API 返回正确数据
- ✅ vue-tsc 零错误
- ✅ Node.js 服务能正常启动
