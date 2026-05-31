# 模块 11 — C++ Signal Server 集成（Phase 2 P0）

> **前置阅读**: [全局主规则](../00-master-rules/MASTER_RULES.md) | [架构设计 §4.2](../../ARCHITECTURE_DESIGN.md#42-c-实时消息服务器)
> **配套文档**: [TASKS.md](./TASKS.md)

---

## 1. 模块定位

将 C++ backend-server 从"基础设施已搭建但未组装"状态推进到"可运行的实时消息处理核心"。P0 阶段目标：

- C++ 服务器通过 TCP 二进制协议接收消息（弹幕/礼物）
- 执行 AC 自动机敏感词过滤
- 持久化到 MySQL
- 通过 **Redis PUBLISH** 将处理结果发布给 Node.js Gateway
- Node.js 通过 **RedisBridge** 订阅 C++ 输出并转发到 WebSocket

**P0 范围**：C++ → Redis 方向（C++ Pub, Node.js Sub）。C++ 订阅 Redis 方向（P1）暂不实现。

---

## 2. 文件变更清单

### C++ 端

| 操作 | 文件路径 | 说明 |
|------|----------|------|
| **新建** | `include/data/mysql_repository.h` | MySQL DataRepository 实现头文件 |
| **新建** | `src/data/mysql_repository.cpp` | MySQL DataRepository 实现 |
| **新建** | `include/data/redis_pubsub.h` | Redis Pub/Sub 模块头文件 |
| **新建** | `src/data/redis_pubsub.cpp` | Redis Pub/Sub 模块（PUBLISH） |
| **修改** | `src/main.cpp` | 组装所有服务，注册消息处理器 |
| **修改** | `src/net/epoll_server.cpp` | handleRead 集成协议解码和消息分发 |
| **修改** | `include/net/epoll_server.h` | 添加消息处理回调 |
| **修改** | `include/util/config_manager.h` | 补充 room/filter 配置字段 |
| **修改** | `src/util/config_manager.cpp` | 实现 room/filter 配置解析 |
| **修改** | `CMakeLists.txt` | 添加新源文件 |

### Node.js 端

| 操作 | 文件路径 | 说明 |
|------|------|------|
| **新建** | `api-server/src/services/redisBridge.js` | Redis Pub/Sub 桥接 |
| **修改** | `api-server/src/server.js` | 初始化 redisBridge |
| **修改** | `api-server/src/services/socket.js` | 瘦身：移除弹幕/礼物直接写 DB |

---

## 3. Redis Pub/Sub 频道设计

```
频道命名规范: {domain}:{action}
────────────────────────────────
danmaku:output      → C++ PUBLISH → Node.js SUBSCRIBE → WS广播
gift:output         → C++ PUBLISH → Node.js SUBSCRIBE → WS广播
danmaku:blocked     → C++ PUBLISH → Node.js SUBSCRIBE → 单播通知发送者
presence:online     → C++ PUBLISH → Node.js SUBSCRIBE → WS广播在线数
```

**消息格式**（JSON 字符串）：

```json
// danmaku:output
{
  "id": 1001,
  "room_id": 1,
  "user_id": 5,
  "username": "viewer_001",
  "content": "主播666！",
  "color": "#00ff41",
  "type": "normal",
  "created_at": "2026-05-24T10:30:00Z"
}

// gift:output
{
  "record_id": 2001,
  "room_id": 1,
  "sender_id": 5,
  "sender_name": "viewer_001",
  "gift_id": 3,
  "gift_name": "🌸",
  "gift_count": 1,
  "total_price": 5.00,
  "effect_type": "rain",
  "created_at": "2026-05-24T10:30:00Z"
}

// danmaku:blocked
{
  "room_id": 1,
  "user_id": 5,
  "original": "违规内容",
  "reason": "matched_level_3_sensitive",
  "matched_words": "xxx"
}
```

---

## 4. 数据模型

### 4.1 MySQL DataRepository 实现

`mysql_repository.h/.cpp` 实现 `chatroom::service::DataRepository` 纯虚接口中的全部方法。需要实现的方法：

| 方法 | SQL 操作 |
|------|----------|
| `queryUserById` | `SELECT * FROM users WHERE id = ?` |
| `queryUserByName` | `SELECT * FROM users WHERE username = ?` |
| `updateUserToken` | `UPDATE users SET token = ? WHERE id = ?` |
| `createRoom` | `INSERT INTO rooms (room_name, host_id, host_name, state) VALUES (...)` |
| `updateRoomState` | `UPDATE rooms SET state = ? WHERE id = ?` |
| `updateRoomOnlineCount` | `UPDATE rooms SET online_count = ? WHERE id = ?` |
| `queryRoomList` | `SELECT * FROM rooms ... LIMIT ? OFFSET ?` |
| `queryRoomById` | `SELECT * FROM rooms WHERE id = ?` |
| `addRoomMember` | `INSERT IGNORE INTO room_members (...) VALUES (...)` |
| `updateRoomMemberLeaveTime` | `UPDATE room_members SET leave_time = NOW() WHERE ...` |
| `queryRoomOnlineCount` | `SELECT COUNT(*) FROM room_members WHERE room_id = ? AND leave_time IS NULL` |
| `saveDanmaku` | `INSERT INTO danmaku_messages (...) VALUES (...)` → 返回 insertId |
| `queryRecentDanmaku` | `SELECT * FROM danmaku_messages WHERE room_id = ? ORDER BY created_at DESC LIMIT ?` |

### 4.2 关键约束

- ⚠️ **不要修改 `repository.h`**：它是纯虚接口，只新建实现类
- ⚠️ **使用参数化查询**：全部 SQL 使用 `mysql_stmt_*` API，禁止字符串拼接
- ⚠️ **字段名映射**：C++ 用 snake_case 成员名，数据库列名也是 snake_case，无需转换
- ⚠️ **数据库表已存在**：rooms 表使用 `room_name` 字段（不是 `roomName`），`state` 字段为 `ENUM('idle','living','closed')`

---

## 5. Redis Pub/Sub 模块规范

### 5.1 `redis_pubsub.h/.cpp` 接口

```cpp
namespace chatroom {

class RedisPublisher {
public:
    // 初始化：使用配置创建独立的 Redis 连接（不共享池）
    bool init(const std::string& host, int port);

    // 发布消息到指定频道
    bool publish(const std::string& channel, const std::string& message);

    // 发布弹幕输出（自动序列化为 JSON）
    bool publishDanmaku(const DanmakuRecord& record);

    // 发布礼物输出
    bool publishGift(uint64_t roomId, uint64_t senderId, const std::string& senderName,
                     int giftId, const std::string& giftName, int count,
                     double totalPrice, const std::string& effectType);

    // 发布被屏蔽弹幕通知
    bool publishBlocked(uint64_t roomId, uint64_t userId,
                        const std::string& original, const std::string& matchedWords);

private:
    redisContext* m_context = nullptr;
};

} // namespace chatroom
```

### 5.2 关键约束

- ⚠️ **不使用连接池**：PUBLISH 命令不要用连接池（订阅连接不能共享，虽然发布可以共享，但保持独立管理更清晰）
- ⚠️ **JSON 序列化**：使用 nlohmann_json 库（项目 CMakeLists.txt 已依赖）
- ⚠️ **失败不崩溃**：publish 失败记录日志但不抛异常，不影响主流程
- ⚠️ **线程安全**：RedisPublisher 所有方法在主线程调用，不需要 mutex

---

## 6. main.cpp 组装规范

### 6.1 启动流程

```
main() 启动顺序:
1. 加载 JSON 配置（config_manager）
2. 初始化日志（spdlog）
3. 初始化 MySQL 连接池
4. 初始化 Redis 连接池
5. 初始化 RedisPublisher（独立连接）
6. 创建 MysqlRepository 实例（传入 MySQL 连接池）
7. 创建 FilterService 实例（加载敏感词词典）
8. 创建 DanmakuService（传入 repository + filter）
9. 创建 EpollServer
10. 设置 EpollServer 的消息回调（MessageDispatcher 逻辑）
11. 注册消息类型处理器到 MessageDispatcher
12. 启动 EpollServer
13. 信号处理（SIGINT/SIGTERM 优雅关闭）
```

### 6.2 消息处理器注册（P0）

```cpp
// P0 阶段：处理 TCP 客户端发来的消息

dispatcher->registerHandler(MSG_DANMAKU, [&](int fd, Message& msg) {
    // 解析 Protobuf Body → DanmakuBody
    // 调用 danmakuService->processDanmaku(fd, roomId, userId, userName, content)
    // 成功后：redisPublisher.publishDanmaku(record)
});

dispatcher->registerHandler(MSG_JOIN_ROOM, [&](int fd, Message& msg) {
    // 调用 roomService->joinRoom(roomId, userId)
    // redisPublisher.publishPresence(...)
});

dispatcher->registerHandler(MSG_HEARTBEAT, [&](int fd, Message& msg) {
    // 更新连接心跳时间，回复 ACK
});
```

---

## 7. epoll_server 协议集成规范

### 7.1 handleRead 改造

当前 `handleRead` 仅将数据累积到读缓冲区。需要改为：

```cpp
void EpollServer::handleRead(int fd) {
    auto conn = m_connectionManager->getConnection(fd);
    char buf[65536];
    ssize_t n = read(fd, buf, sizeof(buf));
    if (n <= 0) {
        handleError(fd);
        return;
    }
    conn->appendReadBuffer(buf, n);

    // 循环解析：处理缓冲区中所有完整的消息包
    while (true) {
        // 步骤1: 尝试解码消息头（需要至少48字节）
        auto headerResult = MessageCodec::decodeHeader(conn->readBuffer());
        if (!headerResult.isOk()) break; // 数据不完整，等待更多数据

        auto& header = headerResult.value();
        size_t packetLen = HEADER_SIZE + header.bodyLen;
        if (conn->readBuffer().size() < packetLen) break; // Body不完整

        // 步骤2: 完整解码
        auto msgResult = MessageCodec::decode(conn->readBuffer().substr(0, packetLen));
        conn->consumeReadBuffer(packetLen); // 消费已解析的数据

        // 步骤3: 分发消息
        if (msgResult.isOk()) {
            m_dispatcher->dispatch(fd, msgResult.value());
        }
    }
}
```

### 7.2 关键约束

- ⚠️ **不要修改现有 accept 和 handleError 逻辑**
- ⚠️ **只追加协议解析代码**到 handleRead，不删除现有缓冲区累积逻辑
- ⚠️ **粘包处理**：循环解析直到缓冲区不足一个完整包
- ⚠️ **4字节长度前缀**：MessageCodec 使用 `[4字节长度][48字节头][N字节体]` 格式，需要确认当前实现

---

## 8. Node.js 端集成规范

### 8.1 redisBridge.js

```javascript
// 订阅 C++ 发布的 Redis 频道，桥接到 Socket.io
class RedisBridge {
    constructor(io) {
        this.io = io;
        this.subscriber = redis.createClient();
    }

    start() {
        this.subscriber.subscribe('danmaku:output', (msg) => {
            const data = JSON.parse(msg);
            this.io.to(`room-${data.room_id}`).emit('new-danmaku', data);
        });
        this.subscriber.subscribe('gift:output', (msg) => { ... });
        this.subscriber.subscribe('danmaku:blocked', (msg) => { ... });
        this.subscriber.subscribe('presence:online', (msg) => { ... });
    }
}
```

### 8.2 socket.js 瘦身

P0 阶段 Node.js 仍直接处理 WebSocket 消息（C++ 尚未订阅 Redis input）。但移除以下逻辑：

- 移除 `send-danmaku` 中的 **MySQL INSERT 操作**（改为 C++ 负责持久化）
- 移除 `send-gift` 中的 **MySQL INSERT + Redis ZINCRBY 操作**
- 保留 Node.js 的基础校验（登录态、参数合法性）
- 保留 Node.js 的 `send-danmaku`/`send-gift` 处理链（作为 P0 过渡方案）

**说明**：P0 阶段 Node.js 暂不通过 Redis Pub input 转发到 C++（C++ 还没有 Subscriber）。Node.js 与 C++ 并行处理各自的数据源。P1 阶段再统一到 C++。

---

## 9. 集成规则

| 规则 | 内容 |
|------|------|
| **不修改 repository.h** | DataRepository 是纯虚接口，只在新建文件中实现 |
| **不修改现有 connect/accept 逻辑** | epoll_server 中只改 handleRead，不改 handleAccept/handleError |
| **Redis 独立连接** | RedisPublisher 用独立 `redisContext`，不走 RedisPool |
| **JSON 用 nlohmann_json** | 项目已依赖 nlohmann_json，直接 `#include <nlohmann/json.hpp>` |
| **数据库列名匹配** | 现有 rooms 表用 `room_name`(不是roomName)、state 用 ENUM 字符串 |
| **编译在容器内** | 所有 C++ 编译通过 `docker exec chatroom-dev` 在 Ubuntu 容器执行 |
| **每功能完测试** | 每个 Task 完成后立即验证，不累积 |

---

## 10. 关键约束

- ⚠️ **P0 不实现 C++ Redis Subscriber**：C++ 不从 Redis 订阅消息（P1 再做），P0 仅建立 C++ →Redis→Node.js 方向
- ⚠️ **P0 Node.js 保留消息处理链**：WebSocket 消息仍由 Node.js socket.js 处理（作为过渡），C++ 处理 TCP 协议消息
- ⚠️ **不使用 Protobuf**：虽 CMakeLists.txt 依赖 Protobuf，但项目无 .proto 文件。P0 使用 JSON 作为消息序列化格式
- ⚠️ **rooms 表 state 枚举**：值为 `'idle'`、`'living'`、`'closed'`（字符串），不是整数。与 roomService 中的 `RoomState` 枚举（整数0-4）不直接对应
- ⚠️ **Docker 内 Redis 地址**：容器内用 `redis:6379`（Docker 服务名），本地开发用 `127.0.0.1:6379`

---

# 模块 11 — Phase 2 P1：C++ Redis Subscriber + 全链路打通

> **前置阅读**: [Phase 2 P0 规则](#模块-11--c-signal-server-集成phase-2-p0) | [架构设计 §12.3](../../ARCHITECTURE_DESIGN.md#123-phase-2-c-实时层-p0)

---

## 11. P1 模块定位

P1 阶段打通消息全链路：**Node.js → Redis → C++ → Redis → Node.js**，让 C++ 成为弹幕和礼物的**单一处理核心**。

```
P1 完整消息流:
┌──────────┐   WS    ┌──────────┐  PUBLISH  ┌─────────┐  SUBSCRIBE  ┌──────────┐
│ Frontend ├────────→│ Node.js  ├──────────→│  Redis  ├───────────→│  C++     │
│ (Vue)    │←────────│ Gateway  │←──────────│ Pub/Sub │←───────────│  Server  │
└──────────┘   WS    └──────────┘ SUBSCRIBE └─────────┘  PUBLISH   └──────────┘
```

**关键变化**：
- Node.js socket.js 中 `send-danmaku` / `send-gift` **不再直接处理**，改为 PUBLISH 到 Redis input 频道
- C++ RedisSubscriber 订阅 input 频道，处理完成后 PUBLISH 到 output 频道
- Node.js RedisBridge 继续订阅 output 频道，广播到 WebSocket

---

## 12. Redis Input 频道设计（P1 新增）

```
danmaku:input   → Node.js PUBLISH → C++ SUBSCRIBE
gift:input      → Node.js PUBLISH → C++ SUBSCRIBE
```

**消息格式**（JSON 字符串）：

```json
// danmaku:input — Node.js → C++
{
  "room_id": 1,
  "user_id": 5,
  "username": "viewer_001",
  "content": "主播666！",
  "color": "#00ff41",
  "type": "normal",
  "timestamp": 1716512345678
}

// gift:input — Node.js → C++
{
  "room_id": 1,
  "sender_id": 5,
  "sender_name": "viewer_001",
  "gift_id": 3,
  "gift_name": "🌸",
  "gift_count": 1,
  "total_price": 5.00,
  "effect_type": "rain",
  "timestamp": 1716512345678
}
```

---

## 13. C++ RedisSubscriber 规范（P1 新增）

### 13.1 接口定义

```cpp
namespace chatroom {

using InputMessageCallback = std::function<void(const std::string& channel, const std::string& message)>;

class RedisSubscriber {
public:
    RedisSubscriber();
    ~RedisSubscriber();

    bool init(const std::string& host, int port, const std::string& password = "");
    bool subscribe(const std::string& channel);
    bool unsubscribe(const std::string& channel);
    bool start();           // 启动监听线程
    void stop();            // 停止监听线程
    bool isRunning() const;

    void setMessageCallback(InputMessageCallback cb);

private:
    void listenLoop();      // 后台线程：阻塞在 redisGetReply
    redisContext* m_context = nullptr;
    std::atomic<bool> m_running{false};
    std::thread m_listenThread;
    InputMessageCallback m_callback;
};

} // namespace chatroom
```

### 13.2 实现约束
- ⚠️ **专用连接**：使用独立的 `redisContext*`，不共享 RedisPool 或 RedisPublisher 的连接
- ⚠️ **后台线程**：SUBSCRIBE 模式需要阻塞等待 `redisGetReply`，必须在独立线程运行
- ⚠️ **线程安全**：回调执行在后台线程，回调内部需要线程安全（通过 main.cpp 的 handler 设计保证）
- ⚠️ **重连不实现**：断开后简单记录日志，不实现自动重连（简化设计）
- ⚠️ **消息格式**：所有 input 消息为 JSON 字符串，与 output 频道格式一致

---

## 14. main.cpp P1 改造规范（P1 新增）

### 14.1 新增启动步骤（插入在 epoll_server start 之前）

```
启动顺序（P0 + P1 追加）:
...
5a. 初始化 RedisSubscriber
5b. 注册 input 频道回调
    5b.1 danmaku:input → 解析JSON → DanmakuService.processDanmaku → RedisPublisher.publishDanmaku
    5b.2 gift:input → 解析JSON → MySQL存储 → RedisPublisher.publishGift
5c. 启动 RedisSubscriber (start listenLoop)
...
```

### 14.2 danmaku:input 处理器伪代码

```cpp
// C++ 从 Redis `danmaku:input` 收到弹幕消息
// 1. 解析 JSON：room_id, user_id, username, content, color
// 2. 调用 danmakuService->processDanmaku(fd=0, roomId, userId, userName, content)
//    (fd=0 表示来自 Redis, 非 TCP 连接)
// 3. 如果成功：RedisPublisher.publishDanmaku(output)
// 4. 如果被过滤：RedisPublisher.publishBlocked(...)
```

### 14.3 gift:input 处理器伪代码

```cpp
// C++ 从 Redis `gift:input` 收到礼物消息
// 1. 解析 JSON：room_id, sender_id, sender_name, gift_id, gift_name, gift_count, total_price
// 2. 存储到 MySQL gift_record 表（通过 mysql_pool 直连）
// 3. 获取 insert_id 作为 record_id
// 4. 通过 RedisPublisher.publishGift(output) 发布结果
```

---

## 15. Node.js socket.js P1 改造规范（P1 新增）

### 15.1 改造目标

`socket.js` 从"消息处理器"变为"消息转发器"：

| 事件 | P0 行为 | P1 行为 |
|------|---------|---------|
| `send-danmaku` | 限流→过滤→MySQL插入→WS广播 | 限流→PUBLISH到`danmaku:input`→等待C++处理后WS广播 |
| `send-gift` | MySQL插入→Redis排行→WS广播 | PUBLISH到`gift:input`→等待C++处理后WS广播 |
| `join-room` | 不变 | 不变 |
| `typing` | 不变 | 不变 |

### 15.2 实现要点

```javascript
// send-danmaku handler (P1)
socket.on('send-danmaku', async ({ roomId, content, color }) => {
    // 仅保留：认证检查 + 限流检查（Node.js 擅长的快速校验）
    // 移除：敏感词过滤 + MySQL INSERT + WS 广播
    
    // 转发到 Redis，由 C++ 处理
    const publisher = getRedisClient()
    await publisher.publish('danmaku:input', JSON.stringify({
        room_id: roomId,
        user_id: userId,
        username: username,
        content: content,
        color: color || '#00ff41',
        timestamp: Date.now()
    }))
})

// send-gift handler (P1)
socket.on('send-gift', async ({ roomId, giftId, count }) => {
    // 仅保留：认证检查
    // 移除：MySQL INSERT + Redis ZINCRBY + WS 广播
    
    const publisher = getRedisClient()
    await publisher.publish('gift:input', JSON.stringify({
        room_id: roomId,
        sender_id: userId,
        sender_name: username,
        gift_id: giftId,
        gift_count: count || 1,
        timestamp: Date.now()
    }))
})
```

### 15.3 关键约束
- ⚠️ **保留限流器**：`send-danmaku` 的 rateLimiter 检查保留在 Node.js 侧，避免无效消息进入 Redis
- ⚠️ **保留认证检查**：所有消息必须通过 socket.data.userInfo 认证检查
- ⚠️ **保留 join-room/disconnect/typing/ping**：这些事件与 C++ 无关，保持不变
- ⚠️ **online-count 暂保留**：当前由 Node.js 通过 Socket.IO 房间人数计算，暂不迁移到 C++

---

## 16. P1 文件变更清单

### C++ 端

| 操作 | 文件路径 | 说明 |
|------|----------|------|
| **修改** | `include/data/redis_pubsub.h` | 追加 RedisSubscriber 类声明 |
| **修改** | `src/data/redis_pubsub.cpp` | 追加 RedisSubscriber 类实现 |
| **修改** | `src/main.cpp` | 集成 RedisSubscriber，注册 input 处理器 |

### Node.js 端

| 操作 | 文件路径 | 说明 |
|------|----------|------|
| **修改** | `api-server/src/services/socket.js` | 改造 send-danmaku/send-gift 为转发 |

---

## 17. 集成验证（P1）

完整消息链路验证：

```
1. PUBLISH danmaku:input → C++ 收到 → 处理 → PUBLISH danmaku:output ✓
2. PUBLISH gift:input → C++ 收到 → 存储 → PUBLISH gift:output ✓
3. 全链路：Node.js → Redis → C++ → Redis → Node.js ✓
```
---

*Phase 2 P1 遵循以上规则和全局主规则进行开发。*
