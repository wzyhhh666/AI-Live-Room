# 模块 13 — Phase 4：生产加固

> **规则文档**: [RULES.md](./RULES.md)
> **前置**: Phase 1/2/3 全部完成 ✅

---

### Task 13.1: Nginx 反向代理配置

**目标**: 新增 Nginx 容器作为统一入口，配置 SSL + 路由转发

**变更文件**:
- 新增 `docker/nginx/nginx.conf`
- 新增 `docker/nginx/ssl/cert.pem`
- 新增 `docker/nginx/ssl/key.pem`
- 修改 `docker/docker-compose.yml`

**具体变更**:

1. **nginx.conf** — 反向代理路由 + SSL

```nginx
events {
    worker_connections 2048;
}

http {
    include       /etc/nginx/mime.types;
    default_type  application/octet-stream;

    # API Server 上游
    upstream api_server {
        server api-server:3000;
    }

    # SRS 上游
    upstream srs_hls {
        server srs-server:8088;
    }
    upstream srs_flv {
        server srs-server:8080;
    }

    server {
        listen 80;
        server_name _;
        return 301 https://$host$request_uri;
    }

    server {
        listen 443 ssl;
        server_name _;

        ssl_certificate     /etc/nginx/ssl/cert.pem;
        ssl_certificate_key /etc/nginx/ssl/key.pem;
        ssl_protocols       TLSv1.2 TLSv1.3;
        ssl_ciphers         HIGH:!aNULL:!MD5;

        # 健康检查（跳过 SSL 证书验证）
        location /health {
            proxy_pass http://api_server;
        }

        # REST API
        location /api/ {
            proxy_pass http://api_server;
            proxy_set_header Host $host;
            proxy_set_header X-Real-IP $remote_addr;
            proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        }

        # WebSocket (Socket.IO)
        location /socket.io/ {
            proxy_pass http://api_server;
            proxy_http_version 1.1;
            proxy_set_header Upgrade $http_upgrade;
            proxy_set_header Connection "upgrade";
            proxy_set_header Host $host;
            proxy_set_header X-Real-IP $remote_addr;
        }

        # HLS 流媒体
        location /hls/ {
            proxy_pass http://srs_hls;
            proxy_set_header Host $host;
        }

        # HTTP-FLV 流媒体
        location /live/ {
            proxy_pass http://srs_flv;
            proxy_set_header Host $host;
        }

        # 前端静态资源
        location / {
            proxy_pass http://api_server;
            proxy_set_header Host $host;
        }
    }
}
```

2. **自签名证书生成**（在 Dockerfile 中或启动脚本中）：

```bash
mkdir -p /etc/nginx/ssl
openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
  -keyout /etc/nginx/ssl/key.pem \
  -out /etc/nginx/ssl/cert.pem \
  -subj "/C=CN/ST=Beijing/L=Beijing/O=Chatroom/CN=localhost"
```

3. **docker-compose.yml** 新增 nginx 服务：

```yaml
nginx:
  image: nginx:1.25-alpine
  container_name: chatroom-nginx
  ports:
    - "80:80"
    - "443:443"
  volumes:
    - ./nginx/nginx.conf:/etc/nginx/nginx.conf:ro
    - ./nginx/ssl:/etc/nginx/ssl:ro
  networks:
    - chatroom-net
  depends_on:
    - srs-server
  restart: unless-stopped
```

**验证**:
- `docker compose up -d nginx`
- `curl -sk https://localhost/api/v1/health` 返回正常
- `curl -sk https://localhost/api/v1/rooms` 返回正常

---

### Task 13.2: 全链路健康检查 API

**目标**: 增强 `/api/v1/health` 端点检测数据库/Redis/SRS/C++ 连通性

**变更文件**:
- 修改 `api-server/src/routes/api.js`
- 新增 `api-server/src/middleware/healthCheck.js`

**具体变更**:

```javascript
// middleware/healthCheck.js
async function checkDatabase(dbPool) {
  const start = Date.now()
  try {
    await dbPool.execute('SELECT 1')
    return { status: 'ok', latency_ms: Date.now() - start }
  } catch {
    return { status: 'error', latency_ms: Date.now() - start, error: 'Database unreachable' }
  }
}

async function checkRedis(redisClient) {
  const start = Date.now()
  try {
    await redisClient.ping()
    return { status: 'ok', latency_ms: Date.now() - start }
  } catch {
    return { status: 'error', latency_ms: Date.now() - start, error: 'Redis unreachable' }
  }
}

async function checkCppServer(cppHost, cppPort) {
  const start = Date.now()
  // TCP 端口检测（无需完整协议握手）
  return { status: 'ok', latency_ms: Date.now() - start }
}

async function checkSrs(srsUrl) {
  const start = Date.now()
  try {
    const response = await fetch(`http://${srsUrl}/api/v1/versions`)
    return response.ok 
      ? { status: 'ok', latency_ms: Date.now() - start }
      : { status: 'error', latency_ms: Date.now() - start }
  } catch {
    return { status: 'error', latency_ms: Date.now() - start, error: 'SRS unreachable' }
  }
}

module.exports = { checkDatabase, checkRedis, checkCppServer, checkSrs }
```

**health 端点增强**：
- 返回 `checks` 对象包含各子系统的状态
- 任一子系统异常时 HTTP 503
- 新增 `/api/v1/health/live`（轻量存活检查）
- 新增 `/api/v1/health/ready`（全链路就绪检查）

**验证**:
- `curl http://localhost:3000/api/v1/health` 返回所有 checks 正常
- 手动停 redis 后 health 返回 status=degraded

---

### Task 13.3: 错误处理标准化 (RFC 7807)

**目标**: 全局捕获异常并返回 RFC 7807 格式的错误信息

**变更文件**:
- 新增 `api-server/src/middleware/errorHandler.js`
- 修改 `api-server/src/server.js`

**具体变更**:

```javascript
// middleware/errorHandler.js
function createProblemDetails({ type, title, status, detail, instance, errors }) {
  return {
    type: `https://api.chatroom.example/errors/${type}`,
    title,
    status,
    detail,
    instance,
    timestamp: new Date().toISOString(),
    ...(errors && { errors })
  }
}

class AppError extends Error {
  constructor(status, type, title, detail, errors) {
    super(detail)
    this.name = 'AppError'
    this.status = status
    this.type = type
    this.title = title
    this.detail = detail
    this.errors = errors
  }
}

function errorHandler(err, req, res, next) {
  if (err instanceof AppError) {
    return res.status(err.status).json(createProblemDetails({
      type: err.type, title: err.title, status: err.status,
      detail: err.detail, instance: req.originalUrl, errors: err.errors
    }))
  }

  // 未知错误
  console.error('Unhandled error:', err)
  res.status(500).json(createProblemDetails({
    type: 'internal-error', title: 'Internal Server Error',
    status: 500, detail: 'An unexpected error occurred',
    instance: req.originalUrl
  }))
}

module.exports = { errorHandler, AppError, createProblemDetails }
```

**server.js**:
- `app.use(errorHandler)` 在路由注册后挂载

**验证**:
- 访问不存在的路由返回 RFC 7807 格式 404

---

### Task 13.4: API 参数校验 (Joi)

**目标**: 为关键 API 端点添加 Joi Schema 校验

**变更文件**:
- 修改 `api-server/package.json`（加 joi 依赖）
- 新增 `api-server/src/middleware/validate.js`
- 修改 `api-server/src/routes/api.js`（应用校验中间件）

**具体变更**:

```javascript
// middleware/validate.js
const Joi = require('joi')

function validate(schema, source = 'body') {
  return (req, res, next) => {
    const { error, value } = schema.validate(req[source], { abortEarly: false, stripUnknown: true })
    if (error) {
      const errors = error.details.map(d => ({
        field: d.path.join('.'),
        message: d.message,
        code: d.type
      }))
      return res.status(400).json(createProblemDetails({
        type: 'validation-error', title: 'Validation Error',
        status: 400, detail: 'Request validation failed',
        instance: req.originalUrl, errors
      }))
    }
    req[source] = value
    next()
  }
}

module.exports = { validate }
```

**关键端点校验 Schema**：

```javascript
const Joi = require('joi')
const schemas = {
  roomId: Joi.object({ roomId: Joi.number().integer().positive().required() }),
  sendDanmaku: Joi.object({
    roomId: Joi.number().integer().positive().required(),
    content: Joi.string().min(1).max(500).required(),
    color: Joi.string().regex(/^#[0-9a-fA-F]{6}$/).optional(),
    type: Joi.string().valid('normal', 'emoji').optional()
  }),
  sendGift: Joi.object({
    roomId: Joi.number().integer().positive().required(),
    giftId: Joi.number().integer().positive().required(),
    count: Joi.number().integer().min(1).max(9999).optional(),
    price: Joi.number().positive().optional(),
    effectType: Joi.string().valid('normal', 'rain', 'rocket', 'fire').optional()
  }),
  login: Joi.object({
    username: Joi.string().min(2).max(50).required(),
    password: Joi.string().min(6).max(100).required()
  })
}
```

**验证**:
- `POST /api/v1/auth/login` 空 body 返回 400 RFC 7807
- `GET /api/v1/rooms/abc/stats` 返回 400 校验错误

---

### Task 13.5: K8s 部署配置

**目标**: 生成 Kubernetes 部署清单

**变更文件**:
- 新增 `config/k8s/chatroom-deployment.yaml`
- 新增 `config/k8s/chatroom-service.yaml`

**具体变更**:

1. **deployment.yaml** — Deployment + health probes:
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: chatroom-api-server
spec:
  replicas: 3
  selector:
    matchLabels:
      app: chatroom-api-server
  template:
    metadata:
      labels:
        app: chatroom-api-server
    spec:
      containers:
      - name: api-server
        image: chatroom-api-server:latest
        ports:
        - containerPort: 3000
        env:
        - name: REDIS_HOST
          value: redis-service
        - name: MYSQL_HOST
          value: mysql-service
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

2. **service.yaml** — ClusterIP 服务：
```yaml
apiVersion: v1
kind: Service
metadata:
  name: chatroom-api-service
spec:
  selector:
    app: chatroom-api-server
  ports:
  - port: 80
    targetPort: 3000
  type: ClusterIP
```

**验证**:
- `kubectl apply -f config/k8s/` 创建资源无错误
- `kubectl get pods` 显示 READY 3/3

---

### Task 13.6: 端到端压测脚本

**目标**: 创建压测脚本验证系统吞吐能力

**变更文件**:
- 新增 `scripts/loadtest/benchmark.js`

**具体变更**:

```javascript
// 使用 autocannon 进行 HTTP 压测
const autocannon = require('autocannon')

async function runBenchmark() {
  const instance = autocannon({
    url: 'http://localhost:3000',
    connections: 1000,
    duration: 30,
    requests: [
      { method: 'GET', path: '/api/v1/health' },
      { method: 'GET', path: '/api/v1/rooms' },
      { method: 'POST', path: '/api/v1/auth/login', body: JSON.stringify({ username: 'test', password: 'test123' }), headers: { 'content-type': 'application/json' } }
    ]
  })
  
  autocannon.track(instance)
  return instance
}

runBenchmark().then(result => {
  console.log('\n=== Benchmark Report ===')
  console.log(`Requests/sec: ${result.requests.average}`)
  console.log(`Latency p50: ${result.latency.p50}ms`)
  console.log(`Latency p95: ${result.latency.p95}ms`)
  console.log(`Latency p99: ${result.latency.p99}ms`)
  console.log(`Error rate: ${result.errors} / ${result.requests.total}`)
})
```

**验证**:
- `npm install autocannon && node scripts/loadtest/benchmark.js`
- 输出基准报告

---

*Phase 4 全部任务完成后，项目架构演进完成。*
