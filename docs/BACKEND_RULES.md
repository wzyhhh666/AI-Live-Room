# 直播弹幕系统 — 后端/服务端开发规则手册

> **版本**: v1.0
> **最后更新**: 2026-05-17
> **适用角色**: 后端工程师、C++开发工程师、系统架构师
> **核心文档依据**: 00~12号设计文档

---

## 📋 目录

1. [后端架构总览](#1-后端架构总览)
2. [网络层实现规范](#2-网络层实现规范)
3. [协议层实现规范](#3-协议层实现规范)
4. [业务层实现规范](#4-业务层实现规范)
5. [数据层实现规范](#5-数据层实现规范)
6. [并发编程规范](#6-并发编程规范)
7. [核心数据结构实现指南](#7-核心数据结构实现指南)
8. [性能优化实战指南](#8-性能优化实战指南)
9. [安全防护规范](#9-安全防护规范)
10. [测试与调试规范](#10-测试与调试规范)
11. [部署与运维规范](#11-部署与运维规范)
12. [代码审查Checklist](#12-代码审查checklist)

---

## 1. 后端架构总览

### 1.1 架构模式
**Reactor + 线程池 + 消息队列** 高并发架构

```
┌──────────────────────────────────────────────┐
│              客户端 (TCP 长连接)              │
└──────────────────┬───────────────────────────┘
                   │
                   ▼
┌──────────────────────────────────────────────┐
│          网络层 / Reactor 主线程             │
│   epoll监听 / 连接管理 / 收包 / 心跳检测     │
└──────────────────┬───────────────────────────┘
                   │ 完整消息对象
                   ▼
┌──────────────────────────────────────────────┐
│           消息队列 / 任务分发层               │
│  Task{callback, taskId, roomId, userId}      │
└──────────────────┬───────────────────────────┘
                   │ 投递到工作线程
                   ▼
┌──────────────────────────────────────────────┐
│           工作线程池 (N个worker)              │
│  弹幕处理 / 房间更新 / 过滤 / 存储写入       │
└──────────────────┬───────────────────────────┘
                   │
                   ▼
┌──────────────────────────────────────────────┐
│            数据层 / 缓存层                    │
│      MySQL 8.0 + Redis 7.x + 本地索引        │
└──────────────────────────────────────────────┘
```

### 1.2 模块职责与依赖关系
```
┌─────────────┐
│  网络层     │ ──→ 协议层 ──→ 业务层 ──→ 数据层
│ (net/)      │       │         │         │
└─────────────┘       ↓         ↓         ↓
                  Message    Service   Repository
```

**依赖原则（强制）**:
- ✅ 单向依赖：`net → protocol → service → data`
- ❌ 禁止反向依赖
- ❌ 禁止跨层调用
- ✅ 层间通过接口/回调通信

### 1.3 线程模型详解
| 线程类型 | 数量 | 职责 | 注意事项 |
|----------|------|------|----------|
| **主线程(Reactor)** | 1 | epoll事件循环、连接建立/断开 | 禁止阻塞操作 |
| **工作线程池** | CPU核心×2 | 业务逻辑处理 | 可阻塞I/O |
| **定时器线程** | 1 | 心跳超时、空闲清理、定时任务 | 低频执行 |
| **日志线程** | 1(可选) | 异步日志写入 | 高吞吐场景启用 |

### 1.4 启动顺序（必须严格遵守）
```
1. 加载配置文件 (config/app.json)
2. 初始化日志系统 (spdlog)
3. 初始化数据库连接池 (MySQL)
4. 初始化Redis连接池
5. 加载敏感词库 → 构建AC自动机
6. 初始化线程池和任务队列
7. 初始化定时器模块
8. 创建epoll实例 + 绑定端口监听
9. 启动工作线程和定时线程
10. 进入主事件循环，开始接受连接
```

---

## 2. 网络层实现规范

### 2.1 连接管理器 (ConnectionManager)
```cpp
// include/net/connection_manager.h
#pragma once

#include <unordered_map>
#include <shared_mutex>
#include <memory>
#include <functional>

namespace chatroom {
namespace net {

class Connection; // 前向声明

using ConnectCallback = std::function<void(int fd)>;
using DisconnectCallback = std::function<void(int fd)>;
using MessageCallback = std::function<void(int fd, uint16_t msgType,
                                           uint64_t roomId, uint64_t userId,
                                           const std::string& data)>;

class ConnectionManager {
public:
    static ConnectionManager& getInstance();
    
    // 连接生命周期管理
    Result<int> addConnection(int fd, std::shared_ptr<Connection> conn);
    Result<void> removeConnection(int fd);
    std::shared_ptr<Connection> getConnection(int fd);
    
    // 广播能力
    Result<void> broadcast(uint64_t roomId, const std::string& message);
    Result<void> sendTo(int fd, const std::string& message);
    
    // 统计信息
    size_t getActiveConnectionCount() const;
    
    // 回调注册
    void setConnectCallback(ConnectCallback cb);
    void setDisconnectCallback(DisconnectCallback cb);
    
private:
    ConnectionManager() = default;
    
    // 主存储：fd → Connection映射
    std::unordered_map<int, std::shared_ptr<Connection>> m_connections;
    mutable std::shared_mutex m_rwLock;  // 读写锁，读多写少
    
    // 回调函数
    ConnectCallback m_onConnect;
    DisconnectCallback m_onDisconnect;
};

} // namespace net
} // namespace chatroom
```

### 2.2 连接对象 (Connection)
```cpp
// include/net/connection.h
#pragma once

#include <atomic>
#include <string>
#include <chrono>
#include "common/error_code.h"

namespace chatroom {
namespace net {

enum class ConnState : uint8_t {
    UNAUTHED = 0,   // 未认证
    AUTHED = 1,     // 已认证
    KICKED = 2      // 被踢下线
};

class Connection {
public:
    explicit Connection(int fd);
    ~Connection();
    
    // 基础属性
    int getFd() const { return m_fd; }
    ConnState getState() const { return m_state.load(); }
    void setState(ConnState state) { m_state.store(state); }
    
    // 用户绑定（登录后）
    void bindUser(uint64_t userId, const std::string& token, uint32_t role);
    void unbindUser();
    uint64_t getUserId() const { return m_userId.load(); }
    std::string getToken() const;
    uint32_t getUserRole() const { return m_role.load(); }
    
    // 心跳管理
    void updateHeartbeatTime();
    std::chrono::steady_clock::time_point getLastHeartbeatTime() const;
    
    // 房间关联
    void setCurrentRoom(uint64_t roomId);
    uint64_t getCurrentRoomId() const { return m_roomId.load(); }
    
    // 读缓冲区管理
    void appendReadBuffer(const char* data, size_t len);
    std::string& getReadBuffer();
    void clearReadBuffer();
    
private:
    int m_fd;
    std::atomic<ConnState> m_state{ConnState::UNAUTHED};
    
    // 用户信息（登录后填充）
    std::atomic<uint64_t> m_userId{0};
    std::string m_token;
    std::atomic<uint32_t> m_role{0};
    
    // 房间信息
    std::atomic<uint64_t> m_roomId{0};
    
    // 心跳时间戳
    std::chrono::steady_clock::time_point m_lastHeartbeat;
    mutable std::mutex m_heartbeatMutex;
    
    // 读缓冲区（用于粘包/拆包）
    std::string m_readBuffer;
    mutable std::mutex m_bufferMutex;
};

} // namespace net
} // namespace chatroom
```

### 2.3 Epoll服务器 (EpollServer)
```cpp
// include/net/epoll_server.h
#pragma once

#include <sys/epoll.h>
#include <vector>
#include <functional>

namespace chatroom {
namespace net {

class EpollServer {
public:
    EpollServer();
    ~EpollServer();
    
    // 初始化与启动
    Result<void> init(const std::string& host, uint16_t port, int backlog = 1024);
    Result<void> start();
    void stop();
    
    // 事件注册
    Result<void> addFd(int fd, uint32_t events);
    Result<void> modifyFd(int fd, uint32_t events);
    Result<void> removeFd(int fd);
    
private:
    void eventLoop();  // 主事件循环
    void handleAccept();  // 处理新连接
    void handleRead(int fd);  // 处理可读事件
    void handleWrite(int fd);  // 处理可写事件
    void handleError(int fd);  // 处理错误事件
    
    int m_epollFd = -1;
    int m_listenFd = -1;
    std::atomic<bool> m_running{false};
    std::vector<struct epoll_event> m_events(1024);  // 事件数组
    
    constexpr static int MAX_EVENTS = 1024;
};

} // namespace net
} // namespace chatroom
```

### 2.4 粘包/拆包处理器
```cpp
// src/net/packet_processor.cpp
#include "net/connection.h"
#include "protocol/message_codec.h"

Result<void> processReadData(Connection* conn) {
    auto& buffer = conn->getReadBuffer();
    
    while (true) {
        // 1. 检查是否有完整的Length字段(4字节)
        if (buffer.size() < sizeof(uint32_t)) {
            break;  // 数据不足，等待下次读取
        }
        
        // 2. 读取Length（网络字节序转主机序）
        uint32_t packetLen;
        memcpy(&packetLen, buffer.data(), sizeof(uint32_t));
        packetLen = ntohl(packetLen);
        
        // 3. 校验包长度合理性
        if (packetLen < HEADER_SIZE || packetLen > MAX_PACKET_SIZE) {
            logError("Invalid packet length: {}", packetLen);
            return Result<void>::fail(ErrorCode::ProtocolError, "Invalid packet length");
        }
        
        // 4. 检查是否收到完整包
        if (buffer.size() < sizeof(uint32_t) + packetLen) {
            break;  // 包不完整，继续等待
        }
        
        // 5. 提取完整包数据
        std::string packet(buffer.data() + sizeof(uint32_t), packetLen);
        buffer.erase(0, sizeof(uint32_t) + packetLen);
        
        // 6. 解析Header和Body
        auto result = MessageCodec::decode(packet);
        if (!result.isOk()) {
            return result;  // 解析失败
        }
        
        // 7. 投递到消息队列或直接处理
        Message msg = result.value();
        dispatchMessage(conn->getFd(), msg);
    }
    
    return Result<void>::ok({});
}
```

---

## 3. 协议层实现规范

### 3.1 消息头定义 (MessageHeader)
```cpp
// include/protocol/message_header.h
#pragma once

#include <cstdint>
#include <string>

namespace chatroom {
namespace protocol {

constexpr uint32_t MAGIC_NUMBER = 0x48415443;  // "HATC" - Chatroom
constexpr uint16_t PROTOCOL_VERSION = 1;
constexpr size_t HEADER_SIZE = 48;  // 固定48字节

struct MessageHeader {
    uint32_t magic;          // 魔数 (4B)
    uint16_t version;        // 协议版本 (2B)
    uint16_t headerLen;      // 头部长度 (2B) - 固定48
    uint16_t msgType;        // 消息类型 (2B)
    uint16_t flags;          // 标记位 (2B)
    uint64_t seq;            // 序列号 (8B)
    uint64_t roomId;         // 房间号 (8B)
    uint64_t userId;         // 用户号 (8B)
    uint64_t timestamp;      // 时间戳毫秒 (8B)
    uint32_t bodyLen;        // Body长度 (4B)
    
    // 序列化为字节数组（Big-Endian）
    std::vector<uint8_t> serialize() const;
    
    // 从字节数组反序列化
    static Result<MessageHeader> deserialize(const uint8_t* data, size_t len);
    
    // 校验合法性
    bool isValid() const;
};

} // namespace protocol
} // namespace chatroom
```

### 3.2 消息编解码器 (MessageCodec)
```cpp
// include/protocol/message_codec.h
#pragma once

#include "message_header.h"
#include "common/result.h"
#include <string>

namespace chatroom {
namespace protocol {

struct Message {
    MessageHeader header;
    std::string body;  // Protobuf序列化后的字节流
};

class MessageCodec {
public:
    // 编码：Header + Body → 完整数据包
    static Result<std::string> encode(const MessageHeader& header, 
                                       const std::string& protobufBody);
    
    // 解码：完整数据包 → Header + Body
    static Result<Message> decode(const std::string& packetData);
    
    // 快速解析Header（不解析Body，用于路由）
    static Result<MessageHeader> decodeHeaderOnly(const std::string& packetData);
    
private:
    // 主机序→网络序转换
    static uint32_t hostToNetwork32(uint32_t val);
    static uint16_t hostToNetwork16(uint16_t val);
    
    // 网络序→主机序转换
    static uint32_t networkToHost32(uint32_t val);
    static uint16_t networkToHost16(uint16_t val);
};

} // namespace protocol
} // namespace chatroom
```

### 3.3 消息类型枚举
```cpp
// include/protocol/message_type.h
#pragma once

#include <cstdint>

namespace chatroom {
namespace protocol {

enum class MessageType : uint16_t {
    // 系统消息段 (1000-1999)
    MSG_LOGIN = 1001,
    MSG_LOGIN_RESP = 1002,
    MSG_LOGOUT = 1003,
    MSG_KICK = 1004,
    
    // 业务消息段 (2000-2999)
    MSG_DANMAKU = 2001,
    MSG_GIFT = 2002,
    
    // 房间消息段 (3000-3999)
    MSG_JOIN_ROOM = 3001,
    MSG_LEAVE_ROOM = 3002,
    MSG_ROOM_CREATE = 3003,
    MSG_ROOM_CLOSE = 3004,
    MSG_ROOM_STATE_SYNC = 3005,
    
    // 心跳与确认 (9000-9999)
    MSG_HEARTBEAT = 9001,
    MSG_ACK = 9002,
    
    // 错误消息 (5000-5999)
    MSG_ERROR = 5001
};

// 工具函数：判断是否为合法消息类型
bool isValidMessageType(uint16_t type);

// 工具函数：获取消息类型名称（用于日志）
const char* getMessageTypeName(MessageType type);

} // namespace protocol
} // namespace chatroom
```

### 3.4 Protobuf消息体定义
```protobuf
// proto/common.proto
syntax = "proto3";
package chatroom.protocol;

message ErrorBody {
    int32 error_code = 1;
    string error_message = 2;
}

message AckBody {
    uint64 seq = 1;
    bool success = 2;
    string message = 3;
}

// proto/auth.proto
syntax = "proto3";
package chatroom.protocol;

message LoginBody {
    string user_name = 1;
    string password_hash = 2;  // SHA-256 hex string
    string client_version = 3;
}

message LoginRespBody {
    uint64 user_id = 1;
    bool success = 2;
    string token = 3;  // UUID or random 32-byte hex
    string message = 4;
    uint32 role = 5;   // 0=观众, 1=主播, 2=管理员
}

// proto/danmaku.proto
syntax = "proto3";
package chatroom.protocol;

message DanmakuBody {
    uint64 room_id = 1;
    uint64 user_id = 2;
    string user_name = 3;
    string content = 4;
    uint64 sent_at = 5;
}

// proto/room.proto
syntax = "proto3";
package chatroom.protocol;

message RoomStateBody {
    uint64 room_id = 1;
    uint64 host_id = 2;
    uint32 state = 3;  // 0=CREATED, 1=READY, 2=LIVE, 3=OFFLINE, 4=CLOSED
    string title = 4;
}

message JoinRoomBody {
    uint64 room_id = 1;
    uint64 user_id = 2;
}

message LeaveRoomBody {
    uint64 room_id = 1;
    uint64 user_id = 2;
}
```

---

## 4. 业务层实现规范

### 4.1 弹幕服务 (DanmakuService)
```cpp
// include/service/danmaku_service.h
#pragma once

#include "common/result.h"
#include "protocol/message_type.h"
#include <string>
#include <memory>

namespace chatroom {
namespace service {

// 前向声明
class RoomService;
class FilterService;
class DataRepository;

class DanmakuService {
public:
    explicit DanmakuService(std::shared_ptr<RoomService> roomSvc,
                           std::shared_ptr<FilterService> filterSvc,
                           std::shared_ptr<DataRepository> dataRepo);
    
    // 核心接口：处理一条弹幕
    Result<void> processDanmaku(uint64_t roomId, uint64_t userId,
                                const std::string& userName,
                                const std::string& content);
    
    // 内部流程：
    // 1. 校验输入（非空、长度限制）
    // 2. 检查限流（用户级+房间级）
    // 3. 敏感词过滤
    // 4. 构造广播消息
    // 5. 通过RoomService获取在线用户列表
    // 6. 调用ConnectionManager广播
    // 7. 异步写Redis热缓存
    // 8. 异步写MySQL冷存储
    
private:
    Result<void> validateInput(const std::string& content);
    Result<void> checkRateLimit(uint64_t userId, uint64_t roomId);
    Result<std::string> filterContent(uint64_t roomId, const std::string& content);
    Result<void> broadcastDanmaku(uint64_t roomId, const std::string& message);
    Result<void> saveToCache(uint64_t roomId, const std::string& danmakuData);
    Result<void> saveToDatabase(uint64_t roomId, uint64_t userId, 
                                const std::string& content);
    
    std::shared_ptr<RoomService> m_roomService;
    std::shared_ptr<FilterService> m_filterService;
    std::shared_ptr<DataRepository> m_dataRepository;
};

} // namespace service
} // namespace chatroom
```

#### 弹幕处理完整流程实现
```cpp
// src/service/danmaku_service.cpp
Result<void> DanmakuService::processDanmaku(uint64_t roomId, uint64_t userId,
                                             const std::string& userName,
                                             const std::string& content) {
    // Step 1: 输入校验
    auto validateResult = validateInput(content);
    if (!validateResult.isOk()) {
        return validateResult;
    }
    
    // Step 2: 限流检查
    auto rateLimitResult = checkRateLimit(userId, roomId);
    if (!rateLimitResult.isOk()) {
        return rateLimitResult;  // 返回限流错误码
    }
    
    // Step 3: 敏感词过滤
    auto filterResult = filterContent(roomId, content);
    if (!filterResult.isOk()) {
        return filterResult;  // 敏感词拦截
    }
    std::string filteredContent = filterResult.value();
    
    // Step 4: 构造弹幕消息体
    DanmakuBody danmakuBody;
    danmakuBody.set_room_id(roomId);
    danmakuBody.set_user_id(userId);
    danmakuBody.set_user_name(userName);
    danmakuBody.set_content(filteredContent);
    danmakuBody.set_sent_at(getCurrentTimestampMs());
    
    std::string bodyData = danmakuBody.SerializeAsString();
    
    // Step 5 & 6: 广播（同步，保证实时性）
    auto broadcastResult = broadcastDanmaku(roomId, bodyData);
    if (!broadcastResult.isOk()) {
        logError("Failed to broadcast danmaku in room {}: {}", 
                 roomId, broadcastResult.message());
        // 广播失败记录日志，但不阻断主流程
    }
    
    // Step 7: 异步写Redis热缓存（不阻塞主链路）
    std::async(std::launch::async, [this, roomId, bodyData]() {
        saveToCache(roomId, bodyData);
    });
    
    // Step 8: 异步写MySQL冷存储（不阻塞主链路）
    std::async(std::launch::async, [this, roomId, userId, filteredContent]() {
        saveToDatabase(roomId, userId, filteredContent);
    });
    
    return Result<void>::ok({});
}
```

### 4.2 房间服务 (RoomService)
```cpp
// include/service/room_service.h
#pragma once

#include "common/result.h"
#include <vector>
#include <memory>
#include <set>

namespace chatroom {
namespace service {

enum class RoomState : uint32_t {
    CREATED = 0,
    READY = 1,
    LIVE = 2,
    OFFLINE = 3,
    CLOSED = 4
};

struct RoomInfo {
    uint64_t roomId;
    std::string roomName;
    uint64_t hostUserId;
    std::string coverUrl;
    RoomState state;
    int onlineCount;
    uint64_t danmakuCount;
};

class RoomService {
public:
    explicit RoomService(std::shared_ptr<DataRepository> dataRepo);
    
    // 房间CRUD
    Result<uint64_t> createRoom(uint64_t hostUserId, const std::string& roomName);
    Result<void> startLive(uint64_t roomId);
    Result<void> stopLive(uint64_t roomId);
    Result<void> closeRoom(uint64_t roomId);
    
    // 成员管理
    Result<void> joinRoom(uint64_t roomId, uint64_t userId);
    Result<void> leaveRoom(uint64_t roomId, uint64_t userId);
    
    // 查询
    Result<RoomInfo> getRoomInfo(uint64_t roomId);
    Result<std::vector<RoomInfo>> queryRoomList(int page, int pageSize, ...);
    
    // 在线人数管理
    Result<std::set<int>> getOnlineUserFds(uint64_t roomId);
    Result<void> updateOnlineCount(uint64_t roomId, int delta);
    
    // 状态校验
    bool isRoomActive(uint64_t roomId) const;
    bool canSendDanmaku(uint64_t roomId) const;
    
private:
    Result<bool> validateStateTransition(uint64_t roomId, RoomState from, RoomState to);
    void syncOnlineCountToRedis(uint64_t roomId);
    void scheduleMysqlSync(uint64_t roomId);
    
    std::shared_ptr<DataRepository> m_dataRepository;
};

} // namespace service
} // namespace chatroom
```

### 4.3 敏感词过滤服务 (FilterService)
```cpp
// include/service/filter_service.h
#pragma once

#include "common/result.h"
#include <string>
#include <vector>
#include <memory>

namespace chatroom {
namespace service {

struct FilterResult {
    std::string filteredText;     // 过滤后文本
    bool wasBlocked;              // 是否被拦截
    std::vector<std::pair<int, int>> hitPositions;  // 命中位置[(start, len)]
    int maxLevel;                 // 命中的最高敏感等级
};

class FilterService {
public:
    FilterService();
    ~FilterService();
    
    // 初始化：加载词库并构建AC自动机
    Result<void> initialize(const std::string& dictPath);
    
    // 执行过滤
    Result<FilterResult> filterText(uint64_t roomId, const std::string& text);
    
    // 热更新词库（后台构建新树，原子替换）
    Result<void> reloadDictionary(const std::string& newPath);
    
    // 查询当前词库版本
    uint64_t getDictionaryVersion() const;
    
private:
    // AC自动机节点
    struct AcNode {
        std::array<AcNode*, 128> children{};
        AcNode* fail = nullptr;
        bool isEnd = false;
        int wordId = -1;
        int level = 0;  // 敏感等级：1=低, 2=中, 3=高
    };
    
    // 构建Trie树
    AcNode* buildTrie(const std::vector<std::pair<std::string, int>>& words);
    
    // 构建fail指针（BFS）
    void buildFailPointers(AcNode* root);
    
    // AC自动机匹配（一次扫描多模式串）
    FilterResult acMatch(AcNode* root, const std::string& text);
    
    // 原子指针切换（支持无锁读取）
    std::atomic<AcNode*> m_root{nullptr};
    std::atomic<uint64_t> m_version{0};
    std::mutex m_buildMutex;  // 保护构建过程
};

} // namespace service
} // namespace chatroom
```

---

## 5. 数据层实现规范

### 5.1 数据仓库接口 (DataRepository)
```cpp
// include/data/repository.h
#pragma once

#include "common/result.h"
#include <vector>
#include <string>
#include <memory>

namespace chatroom {
namespace data {

struct UserInfo {
    uint64_t id;
    std::string userName;
    std::string passwordHash;
    std::string avatarUrl;
    uint32_t role;
    uint8_t status;
};

struct RoomMemberInfo {
    uint64_t id;
    uint64_t roomId;
    uint64_t userId;
    uint32_t memberRole;
    std::string joinTime;
    std::string leaveTime;
};

class DataRepository {
public:
    virtual ~DataRepository() = default;
    
    // === 用户相关 ===
    virtual Result<UserInfo> queryUserById(uint64_t userId) = 0;
    virtual Result<UserInfo> queryUserByName(const std::string& userName) = 0;
    virtual Result<void> updateUserToken(uint64_t userId, const std::string& token) = 0;
    
    // === 房间相关 ===
    virtual Result<uint64_t> createRoom(uint64_t hostUserId, const std::string& name) = 0;
    virtual Result<void> updateRoomState(uint64_t roomId, uint32_t state) = 0;
    virtual Result<void> updateRoomOnlineCount(uint64_t roomId, int count) = 0;
    virtual Result<std::vector<service::RoomInfo>> queryRoomList(...) = 0;
    
    // === 成员相关 ===
    virtual Result<void> addRoomMember(uint64_t roomId, uint64_t userId) = 0;
    virtual Result<void> updateRoomMemberLeaveTime(uint64_t roomId, uint64_t userId) = 0;
    
    // === 弹幕相关 ===
    virtual Result<void> saveDanmaku(uint64_t roomId, uint64_t userId, 
                                     const std::string& content) = 0;
    virtual Result<std::vector<DanmakuInfo>> queryRecentDanmaku(uint64_t roomId, int count) = 0;
    
    // === 礼物相关 ===
    virtual Result<void> saveGift(uint64_t roomId, uint64_t userId,
                                  const std::string& giftName, int count) = 0;
};

// MySQL实现类
class MysqlRepository : public DataRepository {
public:
    explicit MysqlRepository(const MySqlConfig& config);
    Result<void> connect();
    void disconnect();
    
    // 实现所有DataRepository的纯虚函数...
    
private:
    MySqlPool* m_pool;
};

// Redis操作类（辅助）
class RedisHelper {
public:
    explicit RedisHelper(const RedisConfig& config);
    Result<void> connect();
    
    // 房间缓存操作
    Result<void> cacheRoomInfo(uint64_t roomId, const service::RoomInfo& info);
    Result<service::RoomInfo> getCachedRoomInfo(uint64_t roomId);
    
    // 在线用户集合
    Result<void> addOnlineUser(uint64_t roomId, int fd);
    Result<void> removeOnlineUser(uint64_t roomId, int fd);
    Result<std::set<int>> getOnlineUsers(uint64_t roomId);
    
    // 最近弹幕缓存
    Result<void> pushRecentDanmaku(uint64_t roomId, const std::string& danmakuData);
    Result<std::vector<std::string>> getRecentDanmaku(uint64_t roomId, int count);
    
    // 限流计数器
    Result<bool> checkRateLimit(const std::string& key, int maxCount, int windowSec);
    
    // 排行榜操作
    Result<void> incrementGiftScore(uint64_t roomId, uint64_t userId, int score);
    Result<std::vector<std::pair<uint64_t, int>>> getGiftRanking(uint64_t roomId, int topN);
    
private:
    redisContext* m_context;
};

} // namespace data
} // namespace chatroom
```

### 5.2 SQL语句示例
```cpp
// 查询用户（带参数化查询防SQL注入）
const std::string SQL_QUERY_USER_BY_NAME =
    "SELECT id, user_name, password_hash, avatar_url, role, status "
    "FROM users WHERE user_name = ? LIMIT 1";

// 插入房间成员（使用INSERT ON DUPLICATE KEY UPDATE）
const std::string SQL_UPSERT_ROOM_MEMBER =
    "INSERT INTO room_members (room_id, user_id, member_role, join_time, leave_time) "
    "VALUES (?, ?, ?, NOW(), NULL) "
    "ON DUPLICATE KEY UPDATE "
    "    member_role = VALUES(member_role), "
    "    join_time = VALUES(join_time), "
    "    leave_time = NULL";

// 更新房间在线数（批量更新，每10-30秒一次）
const std::string SQL_UPDATE_ONLINE_COUNT =
    "UPDATE rooms SET online_count = ?, updated_at = NOW() WHERE id = ?";
```

---

## 6. 并发编程规范

### 6.1 线程池实现
```cpp
// include/util/thread_pool.h
#pragma once

#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>

namespace chatroom {
namespace util {

struct Task {
    std::function<void()> callback;
    uint64_t taskId = 0;
    uint64_t roomId = 0;
    uint64_t userId = 0;
    std::chrono::system_clock::time_point createTime;
};

class ThreadPool {
public:
    explicit ThreadPool(size_t threadCount = 0);  // 默认CPU核心数×2
    ~ThreadPool();
    
    // 禁止拷贝和赋值
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    
    // 提交任务
    Result<uint64_t> submit(Task task);
    Result<uint64_t> submit(std::function<void()> callback, 
                            uint64_t roomId = 0, uint64_t userId = 0);
    
    // 状态查询
    size_t getPendingTaskCount() const;
    size_t getThreadCount() const;
    bool isRunning() const;
    
    // 关闭线程池
    void shutdown();
    void shutdownNow();  // 强制关闭，丢弃未完成任务
    
private:
    void workerLoop();  // 工作线程主循环
    
    std::vector<std::thread> m_workers;
    std::queue<Task> m_taskQueue;
    
    std::mutex m_queueMutex;
    std::condition_variable m_condition;
    std::atomic<bool> m_stop{false};
    
    std::atomic<uint64_t> m_nextTaskId{1};
    size_t m_maxQueueSize = 65536;
};

} // namespace util
} // namespace chatroom
```

### 6.2 锁使用规范
```cpp
// 场景1：读多写少 - 使用读写锁
class ConnectionManager {
    mutable std::shared_mutex m_rwLock;
    
    std::shared_ptr<Connection> getConnection(int fd) {
        std::shared_lock<std::shared_mutex> lock(m_rwlock);  // 读锁
        auto it = m_connections.find(fd);
        return (it != m_connections.end()) ? it->second : nullptr;
    }
    
    Result<void> addConnection(int fd, std::shared_ptr<Connection> conn) {
        std::unique_lock<std::shared_mutex> lock(m_rwlock);  // 写锁
        m_connections[fd] = std::move(conn);
        return Result<void>::ok({});
    }
};

// 场景2：简单互斥 - 使用mutex
class Buffer {
    std::mutex m_mutex;
    std::string m_data;
    
    void append(const std::string& newData) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_data.append(newData);
    }
};

// 场景3：原子操作 - 使用std::atomic
class Counter {
    std::atomic<int> m_count{0};
    
    void increment() { m_count.fetch_add(1, std::memory_order_relaxed); }
    int get() const { return m_count.load(std::memory_order_acquire); }
};
```

### 6.3 并发安全原则
✅ **推荐做法**:
- 细粒度锁，避免全局大锁
- 锁持有时间尽可能短
- 使用RAII锁管理（lock_guard/unique_lock）
- 优先使用无锁数据结构（原子变量、并发队列）

❌ **禁止行为**:
- 在持锁期间进行I/O操作
- 嵌套加锁导致死锁风险
- 在锁内调用未知的外部函数
- 忘记解锁（必须使用RAII）

---

## 7. 核心数据结构实现指南

### 7.1 定时器（时间轮）
```cpp
// include/util/timer_wheel.h
#pragma once

#include <vector>
#include <functional>
#include <mutex>
#include <memory>

namespace chatroom {
namespace util{

using TimerCallback = std::function<void()>;

struct TimerTask {
    uint64_t taskId;
    TimerCallback callback;
    int rotation;  // 剩余圈数
};

class TimerWheel {
public:
    explicit TimerWheel(size_t slotCount = 1024, int tickIntervalMs = 100);
    
    // 添加定时任务（timeoutMs毫秒后执行）
    uint64_t addTimer(int timeoutMs, TimerCallback callback);
    
    // 取消定时任务
    bool cancelTimer(uint64_t taskId);
    
    // 时间推进（每tickIntervalMs调用一次）
    void tick();
    
private:
    std::vector<std::vector<TimerTask>> m_slots;
    size_t m_currentSlot = 0;
    std::mutex m_mutex;
    
    uint64_t m_nextTaskId = 1;
    int m_tickIntervalMs;
};

} // namespace util
} // namespace chatroom
```

### 7.2 AC自动机构建示例
```cpp
// src/service/filter_service.cpp (关键部分)

FilterService::AcNode* FilterService::buildTrie(
    const std::vector<std::pair<std::string, int>>& words) {
    
    auto root = new AcNode();
    
    for (const auto& [word, level] : words) {
        AcNode* node = root;
        for (unsigned char ch : word) {
            if (!node->children[ch]) {
                node->children[ch] = new AcNode();
            }
            node = node->children[ch];
        }
        node->isEnd = true;
        node->wordId = m_wordIdCounter++;
        node->level = level;
    }
    
    return root;
}

void FilterService::buildFailPointers(AcNode* root) {
    std::queue<AcNode*> q;
    
    // 第一层fail指向root
    for (auto& child : root->children) {
        if (child) {
            child->fail = root;
            q.push(child);
        }
    }
    
    // BFS构建fail指针
    while (!q.empty()) {
        AcNode* current = q.front();
        q.pop();
        
        for (int i = 0; i < 128; i++) {
            AcNode* child = current->children[i];
            if (!child) continue;
            
            AcNode* fail = current->fail;
            while (fail && !fail->children[i]) {
                fail = fail->fail;
            }
            
            child->fail = fail ? fail->children[i] : root;
            q.push(child);
        }
    }
}
```

---

## 8. 性能优化实战指南

### 8.1 V1.0-V5.0优化路线图

| 阶段 | 目标 | 重点优化项 | 预期提升 |
|------|------|------------|----------|
| **V1.0** | 基础可用 | 主链路跑通 | 功能完整性 |
| **V2.0** | 稳定性 | 连接管理、错误处理 | 可靠性99% |
| **V3.0** | 吞吐优化 | 减少拷贝、批量DB写入 | QPS提升50% |
| **V4.0** | 资源优化 | 内存池、缓冲区复用 | 内存降低30% |
| **V5.0** | 架构扩展 | 分片、多节点 | 支持10倍规模 |

### 8.2 优化检查清单
- [ ] 是否有不必要的内存拷贝？（使用move语义、引用传递）
- [ ] 锁粒度是否足够细？（避免全局锁）
- [ ] 热路径是否有I/O操作？（异步化）
- [ ] 字符串拼接是否高效？（reserve预分配）
- [ ] 容器选择是否合适？（unordered_map vs map）
- [ ] 缓冲区大小是否合理？（避免频繁扩容）

### 8.3 性能监控指标
```cpp
// 性能统计收集器
class PerformanceMonitor {
public:
    void recordLatency(const std::string& operation, double latencyMs);
    void incrementCounter(const std::string& metric);
    
    // 定期输出报告
    void report();
    
private:
    struct Metric {
        std::atomic<uint64_t> count{0};
        std::atomic<double> totalLatency{0};
        std::atomic<double> maxLatency{0};
        std::atomic<double> minLatency{999999};
    };
    
    std::unordered_map<std::string, Metric> m_metrics;
};
```

---

## 9. 安全防护规范

### 9.1 输入验证清单
- [ ] 所有外部输入都有长度上限
- [ ] 消息大小不超过MAX_PACKET_SIZE（如1MB）
- [ ] Header字段magic/version/bodyLen合法性校验
- [ ] Protobuf反序列化异常捕获
- [ ] SQL参数化查询（禁止字符串拼接）
- [ ] Redis key格式验证

### 9.2 认证与授权
```cpp
// 消息处理前的权限检查
Result<void> checkPermission(Connection* conn, MessageType msgType) {
    ConnState state = conn->getState();
    
    switch (msgType) {
        case MessageType::MSG_LOGIN:
        case MessageType::MSG_HEARTBEAT:
            // 未登录也可以发送
            return Result<void>::ok({});
            
        default:
            // 其他消息必须已登录
            if (state != ConnState::AUTHED) {
                return Result<void>::fail(ErrorCode::PermissionDenied, 
                                          "Not authenticated");
            }
            return Result<void>::ok({});
    }
}
```

### 9.3 敏感信息保护
```cpp
// 日志脱敏
void logLoginAttempt(const std::string& userName, bool success) {
    // 不要记录密码！只记录用户名和结果
    logInfo("Login attempt: user={}, success={}", userName, success);
}

// Token生成（使用加密安全的随机数）
std::string generateSecureToken() {
    unsigned char bytes[32];
    RAND_bytes(bytes, sizeof(bytes));  // OpenSSL随机数
    
    std::string token;
    for (int i = 0; i < 32; i++) {
        char buf[3];
        snprintf(buf, sizeof(buf), "%02x", bytes[i]);
        token += buf;
    }
    return token;
}
```

---

## 10. 测试与调试规范

### 10.1 单元测试要求
**必须测试的核心模块**:
- ✅ MessageCodec编码解码
- ✅ Connection状态转移
- ✅ Trie树构建与AC自动机匹配
- ✅ 线程池任务调度
- ✅ 房间状态机
- ✅ Result错误码转换
- ✅ 限流算法正确性

### 10.2 测试用例示例
```cpp
TEST(ProtocolTest, EncodeDecodeRoundTrip) {
    MessageHeader header;
    header.magic = MAGIC_NUMBER;
    header.version = PROTOCOL_VERSION;
    header.msgType = static_cast<uint16_t>(MessageType::MSG_DANMAKU);
    header.seq = 12345;
    header.roomId = 1001;
    header.userId = 2001;
    header.timestamp = getCurrentTimestampMs();
    header.bodyLen = 50;
    
    DanmakuBody body;
    body.set_room_id(1001);
    body.set_user_id(2001);
    body.set_user_name("testuser");
    body.set_content("Hello World!");
    body.set_sent_at(header.timestamp);
    
    std::string bodyData = body.SerializeAsString();
    
    // 编码
    auto encodeResult = MessageCodec::encode(header, bodyData);
    ASSERT_TRUE(encodeResult.isOk());
    
    // 解码
    auto decodeResult = MessageCodec::decode(encodeResult.value());
    ASSERT_TRUE(decodeResult.isOk());
    
    Message decoded = decodeResult.value();
    EXPECT_EQ(decoded.header.magic, header.magic);
    EXPECT_EQ(decoded.header.msgType, header.msgType);
    EXPECT_EQ(decoded.header.roomId, header.roomId);
    
    DanmakuBody decodedBody;
    ASSERT_TRUE(decodedBody.ParseFromString(decoded.body));
    EXPECT_EQ(decodedBody.content(), "Hello World!");
}
```

### 10.3 调试技巧
```bash
# 1. 开启TRACE级别日志
# 修改config/logging.yaml: level: TRACE

# 2. 使用GDB调试
gdb ./chatroom-server
(gdb) break main.cpp:100  # 设置断点
(gdb) run                 # 启动程序
(gdb) print conn->getState()  # 查看变量

# 3. 使用Valgrind检测内存泄漏
valgrind --leak-check=full ./chatroom-server

# 4. 使用perf分析性能瓶颈
perf record -g ./chatroom-server
perf report
```

---

## 11. 部署与运维规范

### 11.1 CMake构建配置
```cmake
# CMakeLists.txt (根目录)
cmake_minimum_required(VERSION 3.26)
project(chatroom-server VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# vcpkg集成
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
find_package(Protobuf REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(yaml-cpp REQUIRED)

# 子目录
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(tools)

# 编译选项
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(chatroom-server PRIVATE -O2 -DNDEBUG -Wall -Wextra -Wpedantic)
else()
    target_compile_options(chatroom-server PRIVATE -g -Wall -Wextra)
endif()
```

### 11.2 配置文件模板
```jsonc
{
  "server": {
    "host": "0.0.0.0",
    "port": 8900,
    "backlog": 1024
  },
  "mysql": {
    "host": "127.0.0.1",
    "port": 3306,
    "user": "chatroom",
    "password": "${MYSQL_PASSWORD}",
    "database": "chatroom_db",
    "pool_min": 4,
    "pool_max": 32
  },
  "redis": {
    "host": "127.0.0.1",
    "port": 6379,
    "pool_min": 2,
    "pool_max": 16
  },
  "log": {
    "level": "INFO",
    "file_path": "logs/chatroom.log",
    "max_file_size_mb": 100,
    "max_files": 10
  }
}
```

### 11.3 Docker部署
```dockerfile
# docker/Dockerfile (已配置完成)
FROM ubuntu:22.04
WORKDIR /workspace
CMD ["/bin/bash"]
```

```yaml
# docker/docker-compose.yml (已配置完成)
services:
  chatroom-server:
    build:
      context: ..
      dockerfile: docker/Dockerfile
    ports:
      - "8900:8900"
  
  mysql:
    image: mysql:8.0
    environment:
      MYSQL_ROOT_PASSWORD: root123
      MYSQL_DATABASE: chatroom_db
  
  redis:
    image: redis:7-alpine
```

---

## 12. 代码审查Checklist

### 12.1 提交前自查
- [ ] 代码编译通过（Debug + Release）
- [ ] 单元测试全部通过
- [ ] 无新增编译警告
- [ ] 符合命名规范（PascalCase/camelCase/m_前缀）
- [ ] 错误处理完整（Result封装，无裸异常）
- [ ] 日志输出符合格式（时间/级别/模块/线程ID/fd）
- [ ] 无硬编码配置项
- [ ] 内存管理正确（智能指针使用恰当）
- [ ] 线程安全（锁的使用正确）
- [ ] 无SQL注入风险（参数化查询）
- [ ] 性能无明显退化（热路径无多余拷贝）

### 12.2 审查重点
| 维度 | 权重 | 检查要点 |
|------|------|----------|
| **正确性** | 40% | 逻辑正确、边界条件、错误处理 |
| **可读性** | 25% | 命名清晰、注释适当、结构简洁 |
| **性能** | 20% | 无明显瓶颈、算法合适、资源控制 |
| **安全性** | 10% | 输入验证、权限检查、敏感信息保护 |
| **规范性** | 5% | 符合项目编码风格、文档更新 |

### 12.3 常见问题速查
❌ **必须拒绝**:
- 内存泄漏（new/delete不配对、循环引用）
- 死锁风险（嵌套加锁顺序不一致）
- 数据竞争（未保护共享变量）
- SQL注入（字符串拼接SQL）
- 缓冲区溢出（未检查长度）

⚠️ **需要讨论**:
- 过度抽象（为了"扩展性"增加不必要的复杂度）
- 过早优化（无压测数据的性能改动）
- 函数过长（超过80行难以理解）

✅ **鼓励采纳**:
- 清晰的接口设计（单一职责）
- 完善的错误处理（所有路径都考虑）
- 有意义的单元测试（覆盖边界情况）
- 适当的性能注释（说明为什么这样实现）

---

## 📚 附录

### A. 快速命令参考
```bash
# 构建
cd chatroom-server && mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . -j$(nproc)

# 运行测试
ctest --output-on-failure

# 启动服务
./chatroom-server ../config/app.json

# Docker方式
cd docker && docker compose up -d && docker exec -it chatroom-dev bash
```

### B. 性能目标值
| 指标 | V1.0目标 | V3.0目标 | V5.0目标 |
|------|----------|----------|----------|
| 并发连接数 | 1万 | 5万 | 50万 |
| 弹幕QPS | 1万 | 10万 | 100万 |
| 平均延迟 | <100ms | <50ms | <20ms |
| 内存/万连接 | <50MB | <20MB | <10MB |

### C. 紧急联系人与升级路径
- **P0级故障**: 立即回滚到上一稳定版本
- **P1级故障**: 30分钟内定位并修复
- **P2级故障**: 当天修复并发布
- **P3级缺陷**: 下一个迭代修复

---

**📌 重要提醒**: 后端是整个系统的核心。所有代码必须严格遵循00~12号设计文档。任何架构变更必须先更新文档再改代码。保持简单、稳定、可维护。
