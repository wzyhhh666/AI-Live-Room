# 模块 12 — Phase 3：功能完善

> **规则文档**: [RULES.md](./RULES.md)
> **前置**: Phase 2 P0 + P1 全部完成 ✅

---

### Task 12.1: 表情按钮 @click 事件 + emoji 弹幕发送

**目标**: RoomView.vue 中 6 个表情按钮添加 `@click` 事件，点击后通过 `send-danmaku` 发送 emoji 类型弹幕

**变更文件**:
- 修改 `frontend/src/views/RoomView.vue`

**具体变更**:

1. 在 `sendDanmaku` 方法旁新增 `sendEmoji` 方法：
```typescript
function sendEmoji(emoji: string) {
  socket.emit('send-danmaku', {
    roomId: route.params.id as string,
    content: emoji,
    color: '#ffdd00',
    type: 'emoji'
  })
}
```

2. 在 6 个表情按钮上添加 `@click="sendEmoji('😀')"` 等事件

**验证**:
- `npm run build` 或 `vue-tsc --noEmit` 零错误
- 点击表情按钮后 WebSocket 发送 `send-danmaku` 事件

---

### Task 12.2: DanmakuCanvas emoji 渲染

**目标**: DanmakuCanvas.vue 识别 type='emoji' 的弹幕，使用 40px 字体 + 弹跳动画渲染

**变更文件**:
- 修改 `frontend/src/components/player/DanmakuCanvas.vue`

**具体变更**:

1. 在 `renderDanmaku` 中判断 `danmaku.type === 'emoji'`：
   - `fontSize = 40`
   - 应用 bounce easing 动画

**验证**:
- `npm run build` 零错误
- Canvas 上 emoji 弹幕显示为大号字体 + 弹跳效果

---

### Task 12.3: sensitive_log 表 + Node.js 审计日志

**目标**: 创建 `sensitive_log` 表并集成到 Node.js 敏感词过滤流程

**变更文件**:
- 修改 `api-server/src/services/database.js`
- 修改 `api-server/src/services/sensitiveFilter.js`

**具体变更**:

1. `database.js` 建表阶段追加：
```sql
CREATE TABLE IF NOT EXISTS sensitive_log (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    room_id INT NOT NULL,
    user_id INT NULL,
    original_content TEXT NOT NULL,
    filtered_content TEXT,
    matched_words VARCHAR(500),
    action ENUM('mask', 'block') NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_created (created_at),
    FOREIGN KEY (room_id) REFERENCES rooms(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
```

2. `sensitiveFilter.js` 导出 `filterWithLog` 方法：
   - 参数: `(roomId, userId, content, action)`
   - 过滤内容
   - 如果匹配到敏感词，写入 `sensitive_log` 表
   - 返回过滤结果

**注意**: Phase 2 P1 已将敏感词过滤流程移到 C++ 侧，但 C++ 侧 `FilterService` 已通过 `isOk()` 返回状态。Node.js 侧的 sensitiveFilter 作为**备用过滤层**保留，用于不经过 C++ 的消息。

**验证**:
- `node -e "require('./src/server')"` 不报错
- MySQL sensitive_log 表存在

---

### Task 12.4: 敏感词库扩充

**目标**: 敏感词库从 3 个占位词扩充到 1000+ 真实常见敏感词

**变更文件**:
- 修改 `api-server/config/sensitive_words.json`
- 修改 `backend-server/config/sensitive_words.txt`

**具体变更**:

1. `sensitive_words.json` 扩充为包含以下分类的完整词库：
   - 政治/历史类 (约 200 词)
   - 暴力/恐怖类 (约 150 词)
   - 色情/低俗类 (约 200 词)
   - 广告/推广类 (约 150 词)
   - 人身攻击类 (约 100 词)
   - 其他敏感词 (约 200 词)

2. `sensitive_words.txt` 保持与 JSON 文件内容一致，每行一个词

**注意**: 实际部署时应根据业务场景调整词库，此处提供的是通用过滤词条。

**验证**:
- Node.js 和 C++ 都能正常加载词库
- C++ FilterService 日志显示 1000+ 词加载成功
- 测试过滤功能正常

---

### Task 12.5: 推流密钥管理 API

**目标**: 推流密钥生成、验证、过期 API

**变更文件**:
- 修改 `api-server/src/routes/stream.js`
- 修改 `api-server/src/services/streamService.js`

**具体变更**:

1. 密钥生成规则: `{roomId}_{uuid_v4_short}_{timestamp_hex}`
2. stream/start API:
   - 校验主播身份（根据 token）
   - 生成推流密钥
   - 写入 `stream_records` 表
   - 返回 RTMP 推流地址 + 密钥
3. stream/stop API:
   - 校验主播身份
   - 更新 `stream_records` 状态为 stopped
   - 设置 end_time
4. stream/info API 增强：
   - 返回推流地址、密钥、状态、开始时间

**注意**: 当前已实现 `POST /api/v1/stream/callback` 和部分 stream 功能。

**验证**:
- `node -e "require('./src/server')"` 不报错
- 测试 stream/start 返回合法的推流密钥
- 测试 stream/stop 正确更新记录

---

### Task 12.6: 房间统计 API

**目标**: `GET /api/v1/rooms/{roomId}/stats` 返回房间统计信息

**变更文件**:
- 新增 `api-server/src/routes/stats.js`
- 修改 `api-server/src/server.js`

**具体变更**:

1. stats.js 路由：
```javascript
router.get('/rooms/:roomId/stats', async (req, res) => {
    // 从数据库查询
    // - danmaku 表: COUNT 弹幕总数
    // - gift_record 表: COUNT 礼物总数, SUM total_price 总价值
    // - stream_records 表: 最近一条推流的 duration
    // - 房间信息: peak_online
    // 返回统计数据
})
```

2. server.js 挂载：
```javascript
const statsRouter = require('./routes/stats')
app.use('/api/v1', statsRouter)
```

**验证**:
- `node -e "require('./src/server')"` 不报错
- API 返回正确的 JSON 格式统计

---

*Phase 3 全部任务完成后，进入 Phase 4（Nginx + 健康检查 + K8s）。*
