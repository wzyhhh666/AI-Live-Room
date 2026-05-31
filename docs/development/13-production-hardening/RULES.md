# 模块 13 — Phase 4：生产加固

> **前置阅读**: [架构设计 §12.5](../../ARCHITECTURE_DESIGN.md#125-phase-4-生产加固-p2) | [全局主规则](../00-master-rules/RULES.md)

---

## 1. 模块定位

Phase 4 在所有功能开发完成后，面向生产环境进行加固。目标是提升系统稳定性、可观测性和可运维性。

**核心变更**：
- Nginx 反向代理：SSL 终结 + 统一入口 + 静态资源服务
- 全链路健康检查：`/api/v1/health` 含 Nginx/SRS/C++/MySQL/Redis 检测
- 错误处理标准化：RFC 7807 Problem Details 格式
- API 参数校验：Joi Schema 校验
- 端到端压测：10K 并发基准报告
- K8s 健康探针：liveness + readiness

---

## 2. Nginx 配置规范

### 2.1 容器部署

- Nginx 容器名: `chatroom-nginx`
- 端口映射: `443:443` (HTTPS), `80:80` (HTTP→HTTPS 重定向)
- 网络: `chatroom-net` 与现有服务互通

### 2.2 反向代理路由

| 路径 | 目标 | 说明 |
|------|------|------|
| `/api/*` | `http://api-server:3000` | REST API 网关 |
| `/socket.io/*` | `http://api-server:3000` | WebSocket 升级代理 |
| `/hls/*` | `http://srs-server:8088` | HLS 流媒体 |
| `/live/*` | `http://srs-server:8080` | HTTP-FLV 流媒体 |
| `/` | `http://frontend:5173` | 前端静态资源 |
| `/health` | `http://api-server:3000` | 健康检查入口 |

### 2.3 SSL 配置

- 自签名证书（开发环境）: `/etc/nginx/ssl/chatroom.crt`
- 证书生成: `openssl req -x509 -nodes -days 365 ...`
- HTTP→HTTPS 301 重定向
- 生产环境应替换为 Let's Encrypt 证书

---

## 3. 全链路健康检查规范

### 3.1 端点

`GET /api/v1/health` → 返回 JSON, HTTP 200/503

```json
{
  "status": "ok",
  "version": "1.0.0",
  "timestamp": "2026-05-25T10:00:00.000Z",
  "checks": {
    "database": { "status": "ok", "latency_ms": 5 },
    "redis": { "status": "ok", "latency_ms": 2 },
    "cpp_server": { "status": "ok", "latency_ms": 1 },
    "srs_server": { "status": "ok", "latency_ms": 3 }
  },
  "uptime_sec": 3600
}
```

### 3.2 C++ Server 健康检查端点

C++ 端新增 TCP 协议消息类型 `MSG_HEALTH_CHECK` (2000)：
- 请求: 空 body
- 响应: `{"status":"ok","uptime_sec":3600,"version":"1.0","db_pool_size":4,"redis_connected":true,"filter_words":523}`

---

## 4. 错误处理规范 (RFC 7807)

所有 API 错误响应格式：

```json
{
  "type": "https://api.chatroom.example/errors/validation-error",
  "title": "Validation Error",
  "status": 400,
  "detail": "Invalid room_id: must be a positive integer",
  "instance": "/api/v1/rooms/abc/stats",
  "timestamp": "2026-05-25T10:00:00.000Z",
  "errors": [
    { "field": "room_id", "message": "must be a positive integer", "code": "invalid_type" }
  ]
}
```

### 内置错误类型

| type suffix | title | HTTP Status |
|-------------|-------|-------------|
| `validation-error` | Validation Error | 400 |
| `unauthorized` | Unauthorized | 401 |
| `forbidden` | Forbidden | 403 |
| `not-found` | Not Found | 404 |
| `rate-limited` | Rate Limited | 429 |
| `internal-error` | Internal Server Error | 500 |

---

## 5. API 参数校验规范

- 使用 **Joi** 进行 Schema 校验
- 每个路由 handler 前插入 `validate(schema)` 中间件
- 校验失败返回 400 RFC 7807 格式错误

---

## 6. K8s 健康探针

```yaml
livenessProbe:
  httpGet:
    path: /api/v1/health/live
    port: 3000
  initialDelaySeconds: 10
  periodSeconds: 10

readinessProbe:
  httpGet:
    path: /api/v1/health/ready
    port: 3000
  initialDelaySeconds: 5
  periodSeconds: 5
```

- `/live`: 仅检查进程存活（轻量）
- `/ready`: 全链路检查（数据库+Redis+C+++SRS）

---

## 7. 压测规范

- 工具: `autocannon` (Node.js) + `wrk` (C) 或 `k6`
- 目标: 10K 并发 WebSocket 连接 + 10K/s 消息吞吐
- 场景: 弹幕发送 → Redis → C++ → Redis → WebSocket 广播
- 报告: 平均延迟 P50/P95/P99 + 错误率 + 吞吐量

---

## 8. 文件变更清单

| 操作 | 文件路径 | 说明 |
|------|----------|------|
| **新增** | `docker/nginx/nginx.conf` | Nginx 主配置 |
| **新增** | `docker/nginx/ssl/cert.pem` | SSL 证书(自签名) |
| **新增** | `docker/nginx/ssl/key.pem` | SSL 密钥 |
| **修改** | `docker/docker-compose.yml` | 加 nginx 服务 |
| **修改** | `api-server/src/routes/api.js` | 增强 health 端点 |
| **新增** | `api-server/src/middleware/errorHandler.js` | RFC 7807 错误中间件 |
| **新增** | `api-server/src/middleware/validate.js` | Joi 校验中间件 |
| **修改** | `api-server/src/server.js` | 挂载中间件 |
| **修改** | `api-server/package.json` | 加 joi 依赖 |
| **新增** | `scripts/loadtest/benchmark.js` | 压测脚本 |
| **新增** | `config/k8s/chatroom-deployment.yaml` | K8s 部署配置 |
| **新增** | `config/k8s/chatroom-service.yaml` | K8s 服务配置 |

---

*Phase 4 遵循以上规则和全局主规则进行开发。*
