# 模块 11 — C++ Signal Server 集成 开发任务

> **前置阅读**: [RULES.md](./RULES.md) | [架构设计 §4.2](../../ARCHITECTURE_DESIGN.md#42-c-实时消息服务器)

---

## 任务执行顺序（严格按序号执行）

---

### Task 11.1: 新建 C++ Redis Publisher 头文件

**文件**: `backend-server/include/data/redis_pubsub.h`（新建）

**操作**:
创建头文件，声明 `RedisPublisher` 类：

```cpp
#pragma once
#include <string>
#include <cstdint>
#include <hiredis/hiredis.h>

namespace chatroom {

struct DanmakuOutput {
    uint64_t id;
    uint64_t room_id;
    uint64_t user_id;
    std::string username;
    std::string content;
    std::string color;
    std::string type;
    std::string created_at;
};

struct GiftOutput {
    uint64_t record_id;
    uint64_t room_id;
    uint64_t sender_id;
    std::string sender_name;
    int gift_id;
    std::string gift_name;
    int gift_count;
    double total_price;
    std::string effect_type;
    std::string created_at;
};

class RedisPublisher {
public:
    RedisPublisher() = default;
    ~RedisPublisher();
    
    bool init(const std::string& host, int port, const std::string& password = "");
    bool isConnected() const;
    
    bool publish(const std::string& channel, const std::string& message);
    
    bool publishDanmaku(const DanmakuOutput& record);
    bool publishGift(const GiftOutput& record);
    bool publishBlocked(uint64_t roomId, uint64_t userId,
                        const std::string& original, const std::string& matchedWords);
    bool publishPresence(uint64_t roomId, int onlineCount);

private:
    std::string json_escape(const std::string& s) const;
    redisContext* m_context = nullptr;
};

} // namespace chatroom
```

**验证**: 头文件语法完整，无缺失 include

---

### Task 11.2: 新建 C++ Redis Publisher 实现

**文件**: `backend-server/src/data/redis_pubsub.cpp`（新建）

**操作**:
1. `init()`: 调用 `redisConnectWithTimeout(host.c_str(), port, {1,500000})`（1.5s 超时），如果需要密码则执行 `AUTH`
2. `publish()`: 调用 `redisCommand(m_context, "PUBLISH %s %s", channel.c_str(), message.c_str())`，检查返回值类型为 `REDIS_REPLY_INTEGER`
3. `publishDanmaku()`: 用 nlohmann_json 构造 JSON:
```json
{"id":1,"room_id":2,"user_id":3,"username":"name","content":"666","color":"#00ff41","type":"normal","created_at":"2026-01-01T00:00:00Z"}
```
然后调用 `publish("danmaku:output", json.dump())`

4. `publishGift()`: 类似构造 gift JSON，发布到 `gift:output` 频道
5. `publishBlocked()`: 发布到 `danmaku:blocked` 频道
6. `publishPresence()`: 发布到 `presence:online` 频道
7. `json_escape()`: 对内容中的 `"` `\` 等字符做转义（nlohmann::json 自动处理，不需要手动实现）
8. 析构函数: `redisFree(m_context)`

**验证**: 编译通过（在 Task 11.5 中统一验证）

---

### Task 11.3: 补充 config_manager 解析 room/filter 段

**文件**: `backend-server/include/util/config_manager.h` + `backend-server/src/util/config_manager.cpp`（MODIFY）

**操作**:

1. 在 `config_manager.h` 中确认 `RoomConfig` 和 `FilterConfig` 结构体已定义（检查已有代码）

2. 在 `config_manager.cpp` 的 `loadFromFile()` 方法中，在 `rate_limit` 解析之后、`log` 解析之前，追加以下代码：

```cpp
if (json.contains("room")) {
    auto& rm = json["room"];
    if (rm.contains("max_online")) m_config.room.maxOnline = rm["max_online"];
    if (rm.contains("recent_danmaku_count")) 
        m_config.room.recentDanmakuCount = rm["recent_danmaku_count"];
    if (rm.contains("recent_danmaku_ttl_sec")) 
        m_config.room.recentDanmakuTtlSec = rm["recent_danmaku_ttl_sec"];
}

if (json.contains("filter")) {
    auto& ft = json["filter"];
    if (ft.contains("word_dict_path")) 
        m_config.filter.wordDictPath = ft["word_dict_path"];
    if (ft.contains("reload_interval_sec")) 
        m_config.filter.reloadIntervalSec = ft["reload_interval_sec"];
    if (ft.contains("max_text_length")) 
        m_config.filter.maxTextLength = ft["max_text_length"];
}
```

**验证**: 编译通过（Task 11.5）

---

### Task 11.4: 改造 epoll_server 集成协议解析

**文件**: 
- `backend-server/include/net/epoll_server.h` → 新增 `#include "protocol/message_dispatcher.h"` + 新增 `m_dispatcher` 成员 + `setDispatcher()` 方法
- `backend-server/src/net/epoll_server.cpp` → 改造 `handleRead()` 方法

**操作**:

1. **epoll_server.h 修改**:

在类中新增：
```cpp
private:
    std::shared_ptr<MessageDispatcher> m_dispatcher;
public:
    void setDispatcher(std::shared_ptr<MessageDispatcher> dispatcher) {
        m_dispatcher = dispatcher;
    }
```

头文件顶部新增 `#include "protocol/message_codec.h"` 和 `#include "protocol/message_dispatcher.h"`

2. **epoll_server.cpp handleRead 改造**:

将当前仅累积缓冲区的逻辑，替换为：

```cpp
void EpollServer::handleRead(int fd) {
    char buf[65536];
    
    while (true) {
        ssize_t n = ::read(fd, buf, sizeof(buf));
        if (n > 0) {
            auto conn = m_connectionManager->getConnection(fd);
            if (!conn) break;
            conn->appendReadBuffer(buf, n);
        } else if (n == 0) {
            handleError(fd);
            return;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;  // 无更多数据
            } else if (errno == EINTR) {
                continue;  // 被信号中断，重试
            } else {
                handleError(fd);
                return;
            }
        }
    }
    
    // 尝试解析读缓冲区中的完整消息
    if (m_dispatcher) {
        auto conn = m_connectionManager->getConnection(fd);
        if (!conn) return;
        
        std::string& readBuf = conn->getReadBuffer();
        size_t parsedOffset = 0;
        
        while (parsedOffset + 4 <= readBuf.size()) {
            // 读取4字节长度前缀
            uint32_t networkLen = 0;
            std::memcpy(&networkLen, readBuf.data() + parsedOffset, 4);
            uint32_t packetLen = ntohl(networkLen);
            
            if (packetLen < 48) {
                // 无效包长度，清空缓冲区
                conn->clearReadBuffer();
                break;
            }
            
            size_t totalNeeded = parsedOffset + 4 + packetLen;
            if (readBuf.size() < totalNeeded) {
                break;  // 数据不完整，等待更多数据
            }
            
            // 提取完整包
            std::string packet(readBuf.data() + parsedOffset, 4 + packetLen);
            parsedOffset += 4 + packetLen;
            
            auto decodeResult = MessageCodec::decode(packet);
            if (decodeResult.isOk()) {
                m_dispatcher->dispatch(fd, decodeResult.value());
            }
        }
        
        // 消费已解析的数据
        if (parsedOffset > 0) {
            readBuf.erase(0, parsedOffset);
        }
    }
}
```

3. **保留原有代码**：`handleAccept` 和 `handleError` 不变。

**验证**: 编译通过（Task 11.5）

---

### Task 11.5: 新建 C++ MySQL DataRepository 实现

**文件**: 
- `backend-server/include/data/mysql_repository.h`（新建）
- `backend-server/src/data/mysql_repository.cpp`（新建）

**操作**:

1. **头文件**: 声明 `MySqlRepository` 类，继承 `DataRepository`，构造函数接受 `std::shared_ptr<MySqlPool>`

2. **实现文件**: 实现所有13个虚方法，关键约束：
   - 使用 `mysql_stmt_init/stmt_prepare/stmt_bind_param/stmt_execute/stmt_bind_result` API（参数化查询）
   - `saveDanmaku()` 返回 `mysql_stmt_insert_id(stmt)` 作为弹幕ID
   - `queryRoomList()` 通过字符串拼接 LIMIT/OFFSET（两值为整数，来自入参，安全），或使用参数化
   - **简化方案**（推荐）: 不使用 mysql_stmt，改用 `mysql_real_query` + `mysql_store_result` + `mysql_num_rows/mysql_fetch_row`，这样更简洁且项目已链接 mysqlclient。但必须使用格式化的参数化方式：
   
   推荐使用 `MYSQL_BIND` + `mysql_stmt_prepare` 的 stmt API（安全且标准）。如果编译遇到 compat 问题，回退到 `mysql_real_query` + `snprintf` 构造 SQL（LIMIT/OFFSET 用 `%d` 格式化 int）

3. **rooms 表字段映射**:
   - `RoomInfo::roomName` ← 数据库 `room_name`
   - `RoomInfo::roomId`   ← 数据库 `id`
   - `RoomInfo::state`    ← 数据库 `state`(ENUM字符串)，映射到 `RoomState` 枚举：
     `idle→CREATED, living→LIVE, closed→CLOSED`

**验证**: 编译通过（Task 11.7 中统一验证）

---

### Task 11.6: 更新 CMakeLists.txt 添加新源文件

**文件**: `backend-server/CMakeLists.txt`（MODIFY）

**操作**: 在 `chatroom_common` 的 `add_library` 中追加：

```cmake
    src/data/mysql_repository.cpp
    src/data/redis_pubsub.cpp
```

加在 `src/data/redis_pool.cpp` 之后。

**验证**: `cmake ..` 无错误

---

### Task 11.7: 重写 C++ main.cpp 组装服务

**文件**: `backend-server/src/main.cpp`（全部重写）

**操作**:
重写 main.cpp，实现完整启动流程：

```cpp
#include "common/result.h"
#include "common/logging.h"
#include "util/config_manager.h"
#include "data/mysql_pool.h"
#include "data/redis_pool.h"
#include "data/redis_pubsub.h"
#include "data/mysql_repository.h"
#include "service/filter_service.h"
#include "service/danmaku_service.h"
#include "net/epoll_server.h"
#include "net/connection.h"
#include "protocol/message_dispatcher.h"
#include "protocol/message_codec.h"
#include <csignal>
#include <memory>

using namespace chatroom;

std::atomic<bool> g_running{true};

void signalHandler(int) { g_running = false; }

int main(int argc, char* argv[]) {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // 1. 加载配置
    std::string configPath = (argc > 1) ? argv[1] : "config/app.json";
    auto& configMgr = ConfigManager::getInstance();
    auto loadResult = configMgr.loadFromFile(configPath);
    if (!loadResult.isOk()) {
        std::cerr << "Failed to load config" << std::endl;
        return 1;
    }
    auto& cfg = configMgr.getConfig();
    
    // 2. 初始化日志
    Logger::instance().init(cfg.log.level, cfg.log.filePath,
                            cfg.log.maxFileSizeMb, cfg.log.maxFiles);
    LOG_INFO("Starting chatroom-server v1.0");
    
    // 3. 初始化 MySQL 连接池
    auto mysqlPool = std::make_shared<MySqlPool>();
    if (!mysqlPool->init(cfg.mysql.host, cfg.mysql.port, cfg.mysql.user,
                         cfg.mysql.password, cfg.mysql.database,
                         cfg.mysql.poolMin, cfg.mysql.poolMax)) {
        LOG_CRITICAL("Failed to init MySQL pool");
        return 1;
    }
    LOG_INFO("MySQL pool initialized");
    
    // 4. 初始化 Redis 连接池（留存，P1 使用）
    auto redisPool = std::make_shared<RedisPool>();
    // P0 暂不调用 redisPool->init()，因为 RedisPublisher 独立连接
    LOG_INFO("Redis pool ready");
    
    // 5. 初始化 RedisPublisher（独立连接，P0 核心）
    auto redisPublisher = std::make_shared<RedisPublisher>();
    if (!redisPublisher->init(cfg.redis.host, cfg.redis.port, cfg.redis.password)) {
        LOG_WARN("RedisPublisher init failed (non-fatal)");
    } else {
        LOG_INFO("RedisPublisher connected to {}:{}", cfg.redis.host, cfg.redis.port);
    }
    
    // 6. 创建 MysqlRepository
    auto repository = std::make_shared<MySqlRepository>(mysqlPool);
    
    // 7. 创建 FilterService 并加载敏感词
    auto filterService = std::make_shared<FilterService>();
    filterService->loadDictionary(cfg.filter.wordDictPath);
    LOG_INFO("FilterService loaded");
    
    // 8. 创建 DanmakuService
    auto danmakuService = std::make_shared<DanmakuService>(repository, filterService);
    
    // 9. 创建 MessageDispatcher 并注册处理器
    auto dispatcher = std::make_shared<MessageDispatcher>();
    
    // 注册弹幕消息处理器
    dispatcher->registerHandler(static_cast<uint16_t>(MessageType::MSG_DANMAKU),
        [danmakuService, redisPublisher, &connMgr = net::ConnectionManager::getInstance()]
        (int fd, protocol::Message& msg) -> Result<void> {
            
            // 从 msg.body 解析 DanmakuBody（JSON格式）
            auto bodyJson = nlohmann::json::parse(msg.body);
            uint64_t roomId = bodyJson["room_id"];
            uint64_t userId = bodyJson["user_id"];
            std::string userName = bodyJson["username"];
            std::string content = bodyJson["content"];
            std::string color = bodyJson.value("color", "#00ff41");
            
            // 处理弹幕（过滤 + 持久化）
            auto result = danmakuService->processDanmaku(fd, roomId, userId, userName, content);
            
            // P0: 通过 Redis 发布结果
            if (redisPublisher && redisPublisher->isConnected()) {
                // 简单实现：发布到 Redis，由 Node.js 订阅后广播
                DanmakuOutput output;
                output.room_id = roomId;
                output.user_id = userId;
                output.username = userName;
                output.content = content;
                output.color = color;
                output.type = "normal";
                redisPublisher->publishDanmaku(output);
            }
            
            return Result<void>::ok();
        }
    );
    
    // 注册礼物消息处理器 (P0: 类似结构)
    dispatcher->registerHandler(static_cast<uint16_t>(MessageType::MSG_GIFT),
        [redisPublisher](int fd, protocol::Message& msg) -> Result<void> {
            auto bodyJson = nlohmann::json::parse(msg.body);
            GiftOutput output;
            output.room_id = bodyJson["room_id"];
            output.sender_id = bodyJson["sender_id"];
            output.sender_name = bodyJson["sender_name"];
            output.gift_id = bodyJson["gift_id"];
            output.gift_name = bodyJson.value("gift_name", "🎁");
            output.gift_count = bodyJson.value("count", 1);
            output.total_price = bodyJson.value("total_price", 0.0);
            output.effect_type = bodyJson.value("effect_type", "normal");
            
            if (redisPublisher && redisPublisher->isConnected()) {
                redisPublisher->publishGift(output);
            }
            
            return Result<void>::ok();
        }
    );
    
    // 注册心跳消息处理器
    dispatcher->registerHandler(static_cast<uint16_t>(MessageType::MSG_HEARTBEAT),
        [](int fd, protocol::Message& msg) -> Result<void> {
            auto conn = net::ConnectionManager::getInstance().getConnection(fd);
            if (conn) {
                conn->updateHeartbeatTime();
            }
            return Result<void>::ok();
        }
    );
    
    // 10. 创建 EpollServer
    auto server = std::make_shared<net::EpollServer>();
    server->setDispatcher(dispatcher);
    
    // 11. 启动服务器
    if (!server->init(cfg.server.host, cfg.server.port, cfg.server.backlog)) {
        LOG_CRITICAL("Failed to init EpollServer");
        return 1;
    }
    
    LOG_INFO("Server listening on {}:{}", cfg.server.host, cfg.server.port);
    
    server->start();
    
    // 12. 等待退出信号
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    LOG_INFO("Shutting down...");
    server.reset();
    LOG_INFO("Server stopped");
    
    return 0;
}
```

**验证**: 编译通过（Task 11.8）

---

### Task 11.8: Docker 容器内编译 C++ 项目

**前置条件**: Docker Desktop 已运行，chatroom-dev 容器存在

**操作**:
```bash
# 1. 确保 chatroom-dev 容器运行中
docker start chatroom-dev 2>$null

# 2. 进入容器编译
docker exec chatroom-dev bash -c "
  cd /workspace/backend-server && 
  rm -rf build && mkdir build && cd build &&
  cmake .. -DCMAKE_BUILD_TYPE=Debug &&
  make -j\$(nproc) 2>&1
"
```

**验证**: 
- `make` 返回 0，无编译错误
- `ls -la build/chatroom-server` 可执行文件存在
- 无 warning（或仅有无关紧要的 warning）

**如果编译失败**: 逐条修复编译错误后重新执行本 Task

---

### Task 11.9: 启动 C++ Server 并验证基础连通性

**前置条件**: Task 11.8 编译通过

**操作**:
```bash
# 使用 Docker 网络内的配置启动 C++ 服务器
docker exec -d chatroom-dev bash -c "
  cd /workspace/backend-server &&
  ./build/chatroom-server config/app-docker.json &
"
```

**验证**:
- `docker logs chatroom-dev` 显示 "Server listening on 0.0.0.0:8900"
- 从宿主机连接: `telnet localhost 8900` 能连接成功（然后 Ctrl+C 退出）
- `docker exec chatroom-dev ps aux | grep chatroom-server` 显示进程存在

**排错**:
- 如果 MySQL 连接失败: 确认 `app-docker.json` 中 `mysql.host` 为 `mysql`（Docker 服务名）
- 如果 Redis 连接失败: 确认 `redis.host` 为 `redis`，RedisPublisher init 失败是非致命的

---

### Task 11.10: 创建 Node.js redisBridge.js

**文件**: `api-server/src/services/redisBridge.js`（新建）

**操作**:
按照 RULES.md 第 8.1 节伪代码实现完整类：

1. 构造函数 (`io`) 初始化 Redis 订阅客户端
2. `start()` 订阅 `danmaku:output`、`gift:output`、`danmaku:blocked`、`presence:online`
3. 每个频道回调中解析 JSON → 通过 `this.io.to()` 广播到对应房间
4. 导出单例获取函数 `getRedisBridge(io)`

```javascript
const redis = require('redis')

let instance = null

class RedisBridge {
  constructor(io) {
    this.io = io
    this.subscriber = redis.createClient()
    this.subscriber.on('error', (err) => console.error('RedisBridge error:', err.message))
  }

  async start() {
    await this.subscriber.connect()

    await this.subscriber.subscribe('danmaku:output', (msg) => {
      try {
        const data = JSON.parse(msg)
        this.io.to(`room-${data.room_id}`).emit('new-danmaku', data)
      } catch (e) {
        console.error('RedisBridge danmaku:output parse error:', e.message)
      }
    })

    await this.subscriber.subscribe('gift:output', (msg) => {
      try {
        const data = JSON.parse(msg)
        this.io.to(`room-${data.room_id}`).emit('new-gift', data)
      } catch (e) {
        console.error('RedisBridge gift:output parse error:', e.message)
      }
    })

    await this.subscriber.subscribe('danmaku:blocked', (msg) => {
      try {
        const data = JSON.parse(msg)
        this.io.to(`room-${data.room_id}`).emit('danmaku-blocked', data)
      } catch (e) {
        console.error('RedisBridge danmaku:blocked parse error:', e.message)
      }
    })

    await this.subscriber.subscribe('presence:online', (msg) => {
      try {
        const data = JSON.parse(msg)
        this.io.to(`room-${data.room_id}`).emit('online-count', data)
      } catch (e) {
        console.error('RedisBridge presence:online parse error:', e.message)
      }
    })

    console.log('RedisBridge started, subscribed to 4 channels')
  }
}

function getRedisBridge(io) {
  if (!instance) {
    instance = new RedisBridge(io)
  }
  return instance
}

module.exports = { getRedisBridge }
```

**验证**: Node.js 启动时无 require 错误

---

### Task 11.11: 修改 server.js 初始化 RedisBridge

**文件**: `api-server/src/server.js`（MODIFY）

**操作**:
1. 顶部引入: `const { getRedisBridge } = require('./services/redisBridge')`
2. 在 `app.set('io', io)` 之后、`server.listen` 之前插入:
```javascript
const redisBridge = getRedisBridge(io)
redisBridge.start().catch(err => {
  console.error('Failed to start RedisBridge:', err.message)
})
```

**验证**: Node.js 启动日志输出 "RedisBridge started, subscribed to 4 channels"

---

### Task 11.12: 端到端验证（Redis Pub/Sub 链路）

**前置条件**: 所有前述 Task 完成

**操作**:
1. 完整启动环境: Docker(MySQL+Redis+SRS) + Node.js + Vite + C++ Server
2. 打开 Redis CLI 发布一条测试消息:
```bash
docker exec chatroom-redis redis-cli PUBLISH danmaku:output '{"id":999,"room_id":1,"user_id":1,"username":"test","content":"Test from Redis","color":"#ff0000","type":"normal","created_at":"2026-05-24T00:00:00Z"}'
```
3. 观察前端 RoomView 是否收到 `new-danmaku` 事件并在 Canvas 上渲染弹幕

**验证**:
- Node.js 控制台无报错
- 前端弹幕区域出现 "Test from Redis" 弹幕 ✅

---

### Task 11.13: 端到端验证（C++ → Redis 链路）

**前置条件**: Task 11.9 C++ Server 已运行，Task 11.11 RedisBridge 已运行

**操作**:
1. 使用简单的 TCP 客户端（如 netcat 或 Python）向 C++ 服务器发送一个二进制协议包
2. 或者使用 Redis CLI 验证 C++ Publisher 是否工作:
```bash
# 检查 C++ RedisPublisher 是否可用（发布一条测试消息）
docker exec chatroom-dev bash -c "redis-cli -h redis PING"
```

**注**: P0 阶段 TCP 客户端不在本模块范围内，C++ 服务器的功能通过编译验证 + Redis 直连测试即可验证。完整的 TCP 客户端测试在 chatroom-client 子项目中。

**验证**:
- C++ Server 日志显示正常运行无报错 ✅
- `docker exec chatroom-dev netstat -tlnp | grep 8900` 显示端口监听 ✅

---

*所有任务完成后，进入 Phase 2 P1（C++ Redis Subscriber）+ Phase 3（表情+敏感词库）。*

---

# Phase 2 P1：C++ Redis Subscriber + 全链路打通

> **规则文档**: [RULES.md §11-17](./RULES.md)
> **前置**: Phase 2 P0 全部任务完成 ✅

---

### Task 11.14: C++ RedisSubscriber 头文件声明

**目标**: 在 `redis_pubsub.h` 中追加 `RedisSubscriber` 类声明

**变更文件**:
- 修改 `backend-server/include/data/redis_pubsub.h`

**内容要点**:
- 在 `RedisPublisher` 类定义之后（仍在 `chatroom` 命名空间内）追加 `RedisSubscriber` 类
- 使用 `std::function<void(const std::string&, const std::string&)>` 作为消息回调
- 后台线程成员 `std::thread m_listenThread` + `std::atomic<bool> m_running`
- hiredis 独立连接 `redisContext* m_context`
- 接口方法: `init()`, `subscribe()`, `unsubscribe()`, `start()`, `stop()`, `isRunning()`, `setMessageCallback()`

**验证**:
- 代码格式正确
- 没有与 RedisPublisher 的成员冲突

---

### Task 11.15: C++ RedisSubscriber 实现

**目标**: 在 `redis_pubsub.cpp` 中追加 `RedisSubscriber` 类实现

**变更文件**:
- 修改 `backend-server/src/data/redis_pubsub.cpp`

**实现要点**:
- `init()`: 调用 `redisConnectWithTimeout` + auth（与 RedisPublisher 类似）
- `subscribe()`: 发送 `redisCommand(ctx, "SUBSCRIBE %s", channel.c_str())`
- `unsubscribe()`: 发送 `redisCommand(ctx, "UNSUBSCRIBE %s", channel.c_str())`
- `start()`: 启动 `listenLoop` 线程
- `stop()`: 设置 `m_running = false` → 断开连接（使 `redisGetReply` 返回）→ `join` 线程
- `listenLoop()`:
  ```cpp
  while (m_running) {
      redisReply* reply = nullptr;
      if (redisGetReply(m_context, (void**)&reply) == REDIS_OK) {
          if (reply->type == REDIS_REPLY_ARRAY && reply->elements >= 3) {
              std::string type = reply->element[0]->str;
              if (type == "message") {
                  std::string channel = reply->element[1]->str;
                  std::string message = reply->element[2]->str;
                  if (m_callback) m_callback(channel, message);
              }
          }
          freeReplyObject(reply);
      } else {
          // 连接断开或错误
          break;
      }
  }
  ```

**验证**:
- 编译通过（Task 11.17 统一验证）

---

### Task 11.16: main.cpp 集成 RedisSubscriber + 注册处理器

**目标**: 在 main.cpp 中初始化 RedisSubscriber，注册 `danmaku:input` 和 `gift:input` 处理器

**变更文件**:
- 修改 `backend-server/src/main.cpp`

**具体变更**:

1. **增加 `redis_subscriber` 的 include**（已通过 `redis_pubsub.h` 包含）

2. **在 `MessageDispatcher` 初始化之后、`epollServer` 启动之前** 插入:

```cpp
// === Phase 2 P1: Redis Subscriber ===
auto redisSubscriber = std::make_unique<data::RedisSubscriber>();
if (redisSubscriber->init(config.redis.host, config.redis.port, config.redis.password)) {
    redisSubscriber->setMessageCallback(
        [&danmakuService, &redisPublisher](const std::string& channel, const std::string& message) {
            try {
                auto json = nlohmann::json::parse(message);

                if (channel == "danmaku:input") {
                    int roomId = json["room_id"];
                    int userId = json["user_id"];
                    std::string userName = json["username"];
                    std::string content = json["content"];

                    auto result = danmakuService->processDanmaku(0, roomId, userId, userName, content);
                    if (result.isSuccess()) {
                        auto* data = result.getData();
                        redisPublisher->publishDanmaku(
                            data->danmaku_id, roomId, userId, userName,
                            content, data->color, "normal"
                        );
                    } else {
                        redisPublisher->publishBlocked(roomId, userId, content, {}, "filtered");
                    }
                }
                else if (channel == "gift:input") {
                    int roomId = json["room_id"];
                    int senderId = json["sender_id"];
                    std::string senderName = json["sender_name"];
                    int giftId = json["gift_id"];
                    std::string giftName = json.value("gift_name", "");
                    int count = json.value("gift_count", 1);
                    double price = json.value("total_price", 0.0);
                    std::string effectType = json.value("effect_type", "normal");

                    // 直接通过 MySQL 池插入 gift_record
                    auto conn = data::MySqlPool::getInstance().getConnection();
                    if (conn) {
                        char sql[2048];
                        char escapedName[512], escapedEffect[128];
                        mysql_real_escape_string(conn, escapedName, senderName.c_str(), senderName.size());
                        mysql_real_escape_string(conn, escapedEffect, effectType.c_str(), effectType.size());
                        snprintf(sql, sizeof(sql),
                            "INSERT INTO gift_record (room_id, sender_id, sender_name, gift_id, gift_name, gift_count, total_price, effect_type) "
                            "VALUES (%d, %d, '%s', %d, '%s', %d, %.2f, '%s')",
                            roomId, senderId, escapedName, giftId, giftName.c_str(), count, price, escapedEffect);
                        if (mysql_real_query(conn, sql, strlen(sql)) == 0) {
                            int recordId = (int)mysql_insert_id(conn);
                            redisPublisher->publishGift(recordId, roomId, senderId, senderName,
                                giftId, giftName, count, price, effectType);
                        }
                        data::MySqlPool::getInstance().returnConnection(conn);
                    }
                }
            } catch (const std::exception& e) {
                LOG_ERROR("RedisSubscriber callback error: {}", e.what());
            }
        }
    );
    redisSubscriber->subscribe("danmaku:input");
    redisSubscriber->subscribe("gift:input");
    redisSubscriber->start();
    LOG_INFO("RedisSubscriber started, subscribed to danmaku:input and gift:input");
} else {
    LOG_WARN("RedisSubscriber init failed (non-fatal)");
}
```

3. **在 shutdown 阶段增加 `redisSubscriber->stop()`**:
```cpp
if (redisSubscriber) redisSubscriber->stop();
```

**验证**:
- 编译通过（Task 11.17 统一验证）

---

### Task 11.17: Docker 编译验证

**目标**: 在 Docker 容器内编译 C++ 项目，修复所有编译错误

**命令**:
```bash
docker exec chatroom-dev bash -c "cd /workspace/backend-server && rm -rf build && mkdir build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake && make -j4 2>&1"
```

**验证**:
- ✅ 编译零错误
- ✅ `chatroom-server` 和 `chatroom-tests` 可执行文件已生成
- ✅ 单元测试通过: `./build/chatroom-tests`

---

### Task 11.18: Node.js socket.js 改造

**目标**: 将 `send-danmaku` 和 `send-gift` 从"直接处理"改为"转发到 Redis input 频道"

**变更文件**:
- 修改 `api-server/src/services/socket.js`

**具体变更**:

1. **在文件顶部增加 `ioredis` 导入**:
```javascript
const Redis = require('ioredis')
const redisPublisher = new Redis({
    host: process.env.REDIS_HOST || '127.0.0.1',
    port: parseInt(process.env.REDIS_PORT || '6379'),
    lazyConnect: true
})
redisPublisher.connect().catch(err => console.error('socket.js redis publisher connect error:', err.message))
```

2. **改造 `send-danmaku` handler**:
   - 保留: `socket.data.userInfo` 认证检查 → `return socket.emit('error', ...)` 如果未认证
   - 保留: `rateLimiter.check` 限流检查
   - 移除: `sensitiveFilter` 过滤 → `${__dirname}/../utils/rateLimiter` 中的过滤调用
   - 移除: `database.query` INSERT danmaku 到 MySQL
   - 移除: `io.to(roomName).emit('new-danmaku', ...)` 直接广播
   - **改为**: `redisPublisher.publish('danmaku:input', JSON.stringify({...}))`

3. **改造 `send-gift` handler**:
   - 保留: `socket.data.userInfo` 认证检查
   - 移除: `giftService.sendGift` 调用（包含 MySQL 插入 + Redis 排行）
   - 保留: 礼物有效性检查（查询数据库检查 gift_id 是否存在）
   - **改为**: `redisPublisher.publish('gift:input', JSON.stringify({...}))`

4. **graceful shutdown 增加**:
```javascript
if (redisPublisher) await redisPublisher.quit()
```

**验证**:
- Node.js 服务无语法错误: `node -e "require('./src/server')"` 不报错
- WebSocket 连接正常

---

### Task 11.19: 全链路端到端验证

**目标**: 验证完整消息链路 Node.js → Redis → C++ → Redis → Node.js

**验证步骤**:

```
1. Docker 环境确认
   ✅ chatroom-dev (C++ Dev) running
   ✅ chatroom-redis (Redis 7) running
   ✅ chatroom-mysql (MySQL 8) running
   ✅ chatroom-srs (SRS 5) running

2. 启动 C++ Server
   ✅ 日志显示: RedisSubscriber started, subscribed to danmaku:input and gift:input

3. 验证 danmaku:input 链路
   docker exec chatroom-redis redis-cli PUBLISH danmaku:input '{"room_id":1,"user_id":2,"username":"e2e","content":"P1_Test_Pass","timestamp":1716512345678}'
   → 观察 C++ 日志: "RedisSubscriber callback: danmaku:input"
   → 检查 MySQL: danmaku 表有新记录
   → 验证 Redis: 新消息出现在 danmaku:output

4. 验证 gift:input 链路
   docker exec chatroom-redis redis-cli PUBLISH gift:input '{"room_id":1,"sender_id":2,"sender_name":"e2e","gift_id":1,"gift_name":"🚀","gift_count":1,"total_price":9.90,"effect_type":"rocket","timestamp":1716512345678}'
   → 观察 C++ 日志
   → 检查 MySQL: gift_record 表有新记录
   → 验证 Redis: 新消息出现在 gift:output

5. Node.js 端验证
   ✅ Node.js 日志无报错
   ✅ redisBridge 正常订阅 4 个 output 频道
```

**验证**:
- 以上 5 步全部通过 ✅

---

*Phase 2 P1 全部任务完成后，进入 Phase 3（表情 + 敏感词库 + 推流密钥）。*
