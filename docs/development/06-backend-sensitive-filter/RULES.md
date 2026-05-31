# 模块 06 — 后端敏感词过滤 开发规则

> **前置模块**: 模块 05 必须通过全部测试

---

## 1. 模块定位

在 Node.js 端实现轻量级 Trie 树敏感词过滤，在弹幕发送前检查内容。

---

## 2. 文件变更

| 操作 | 文件路径 |
|------|----------|
| **新建** | `api-server/src/services/sensitiveFilter.js` |
| **新建** | `api-server/config/sensitive_words.json` |
| **修改** | `api-server/src/services/socket.js` |

---

## 3. 敏感词文件格式

```json
{
  "words": ["敏感词1", "敏感词2", "敏感词3"]
}
```

## 4. 过滤器接口

```javascript
const FilterLevel = { PASS: 0, REPLACE: 1, BLOCK: 2 }

function build(trie, words, level)
function filter(trie, text) → { text: string, blocked: boolean }
```

- REPLACE 级别词：替换为 `*`
- BLOCK 级别词：整条消息拦截，返回 `{ blocked: true }`

---

## 5. Socket 集成

在 `socket.js` 的 `send-danmaku` 处理器中，**限流检查之后**、**roomService.sendDanmaku 之前**：

```javascript
const filterResult = sensitiveFilter.filter(content)
if (filterResult.blocked) {
  socket.emit('error', { message: 'Content blocked' })
  return
}
// 使用 filterResult.text 替换原始 content
```

---

## 6. 关键约束

- ⚠️ 过滤只作用于 Socket 发送路径，不影响 HTTP 直调
- ⚠️ 被拦截的消息不写入数据库、不广播
- ⚠️ 词库文件配置化，从 JSON 加载
- ⚠️ build 在服务器启动时执行一次，之后 filter 零开销

---

*遵循以上规则和全局主规则进行开发。*
