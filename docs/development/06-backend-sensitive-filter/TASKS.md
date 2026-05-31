# 模块 06 — 后端敏感词过滤 开发任务

> **前置条件**: 模块 05 测试通过

---

## Task 6.1: 创建敏感词 JSON 配置

**文件**: `api-server/config/sensitive_words.json`（新建）

包含 5-10 个示例敏感词用于测试

---

## Task 6.2: 创建 sensitiveFilter.js

**文件**: `api-server/src/services/sensitiveFilter.js`（新建）

实现 Trie 树 build + filter 逻辑

---

## Task 6.3: 集成到 socket.js

**文件**: `api-server/src/services/socket.js`

在限流检查之后、sendDanmaku 之前，加入过滤逻辑

---

## Task 6.4: 启动验证

```bash
node src/server.js
```

用包含敏感词的弹幕测试是否被拦截

---

*完成后运行 TESTS.md*
