# 直播弹幕系统 — 前端/客户端开发规则手册

> **版本**: v1.0
> **最后更新**: 2026-05-17
> **适用角色**: 前端工程师、客户端开发工程师、UI/UX设计师
> **服务端对应文档**: 00~12号文档 + 13_客户端体系开发文档.md

---

## 📋 目录

1. [前端架构概述](#1-前端架构概述)
2. [技术栈与工具链](#2-技术栈与工具链)
3. [代码规范](#3-代码规范)
4. [网络层开发规范](#4-网络层开发规范)
5. [协议对接规范](#5-协议对接规范)
6. [UI交互规范](#6-ui交互规范)
7. [状态管理规范](#7-状态管理规范)
8. [弹幕模块开发指南](#8-弹幕模块开发指南)
9. [房间模块开发指南](#9-房间模块开发指南)
10. [本地缓存策略](#10-本地缓存策略)
11. [性能优化指南](#11-性能优化指南)
12. [测试规范](#12-测试规范)
13. [常见问题FAQ](#13-常见问题faq)

---

## 1. 前端架构概述

### 1.1 客户端定位
**客户端是直播系统的用户界面层**，负责：
- 用户交互（输入、点击、展示）
- 协议封装与解析
- 本地状态管理
- 网络通信与重连
- 数据展示与缓存

### 1.2 分层架构
```
┌─────────────────────────────────────┐
│           交互层 (UI Layer)          │
│  输入框 / 列表 / 状态提示 / 弹幕展示  │
└────────────────┬────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────┐
│         应用层 (App Layer)           │
│  登录态 / 页面路由 / 会话控制 / 重连  │
└────────────────┬────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────┐
│        协议层 (Protocol Layer)       │
│  消息封包 / 解包 / 序列化 / 反序列化 │
└────────────────┬────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────┐
│         网络层 (Network Layer)       │
│  TCP连接 / 收发缓冲 / 心跳 / 重连   │
└────────────────┬────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────┐
│      缓存层 (Cache Layer)            │
│  最近房间 / 最近消息 / 未发送队列    │
└─────────────────────────────────────┘
```

### 1.3 线程模型
| 线程 | 职责 | 说明 |
|------|------|------|
| **主线程** | UI事件处理、界面渲染 | 必须保持流畅，禁止阻塞 |
| **网络线程** | Socket收发、协议解析 | 独立线程，避免阻塞UI |
| **定时线程** | 心跳发送、重连检测、缓存刷新 | 低频任务 |
| **业务线程池** | 异步数据处理、展示更新 | 避免主线程卡顿 |

---

## 2. 技术栈与工具链

### 2.1 核心技术栈
| 组件 | 版本/标准 | 用途 |
|------|-----------|------|
| **C++标准** | C++17 | 语言标准（与服务端一致） |
| **编译器** | GCC 13+ / MSVC 2022+ | 编译器 |
| **构建系统** | CMake 3.26+ | 构建工具 |
| **序列化库** | Protobuf 27+ | 消息编解码 |
| **日志库** | spdlog 1.14+ | 日志记录 |
| **JSON库** | nlohmann/json 3.11+ | 配置和调试 |

### 2.2 UI框架选择（根据平台）
| 平台 | 推荐框架 | 特点 |
|------|----------|------|
| Windows桌面 | Qt 6.x / WxWidgets | 原生体验、成熟稳定 |
| 跨平台 | Flutter / Electron | 快速开发、热重载 |
| 移动端 | Qt Quick / Native | 性能优秀 |

### 2.3 开发环境配置
```bash
# Docker开发环境（推荐）
docker exec -it chatroom-dev bash

# 或本地环境
# 安装C++17编译器、CMake、Protobuf编译器
```

---

## 3. 代码规范

### 3.1 命名规范（与服务端保持一致）
| 类型 | 规则 | 示例 |
|------|------|------|
| 类名 | PascalCase | `LoginDialog`, `DanmakuWidget` |
| 函数名 | camelCase, 动词开头 | `handleClick()`, `sendDanmaku()` |
| 成员变量 | `m_`前缀 + camelCase | `m_userId`, `m_roomId` |
| 局部变量 | camelCase | `messageText`, `roomId` |
| 常量 | 全大写下划线 | `MAX_DANMAKU_LENGTH`, `RECONNECT_INTERVAL_MS` |

### 3.2 文件组织结构
```
client/
├── include/
│   ├── ui/                  # UI组件头文件
│   ├── app/                 # 应用逻辑头文件
│   ├── protocol/            # 协议层头文件
│   ├── network/             # 网络层头文件
│   └── cache/               # 缓存层头文件
├── src/
│   ├── main.cpp             # 程序入口
│   ├── ui/                  # UI组件实现
│   ├── app/                 # 应用逻辑实现
│   ├── protocol/            # 协议层实现
│   ├── network/             # 网络层实现
│   └── cache/               # 缓存层实现
├── resources/               # UI资源文件
│   ├── images/
│   ├── fonts/
│   └── styles/
├── config/                  # 配置文件
│   └── client_config.json
├── proto/                   # Protobuf定义（与服务端共享）
└── tests/                   # 测试代码
```

### 3.3 错误处理规范
```cpp
// 使用Result封装（与服务端一致）
Result<bool> login(const std::string& userName, const std::string& password);
Result<void> joinRoom(uint64_t roomId);
Result<void> sendDanmaku(const std::string& content);

// 错误处理示例
auto result = sendDanmaku(content);
if (!result.isOk()) {
    showErrorToUser(result.message());
    // 根据错误码做不同处理
    switch (result.code()) {
        case ErrorCode::RateLimited:
            showWarning("发言太频繁，请稍后再试");
            break;
        case ErrorCode::PermissionDenied:
            showError("没有权限发送弹幕");
            break;
        default:
            showError("发送失败: " + result.message());
    }
}
```

### 3.4 日志规范
**日志格式**（与服务端可联调）：
```
[时间][级别][模块][线程ID][会话ID][消息]
示例: [2026-05-15 10:31:22][INFO][ui][tid=main][session=abc123] 用户点击登录按钮
```

**日志使用场景**：
- ✅ 用户关键操作（登录、进房、发弹幕）
- ✅ 网络事件（连接建立、断开、重连）
- ✅ 错误情况（鉴权失败、限流触发）
- ❌ 不要记录密码、Token等敏感信息

---

## 4. 网络层开发规范

### 4.1 TCP连接管理
```cpp
class TcpClient {
public:
    Result<void> connectServer(const std::string& host, int port);
    Result<void> disconnect();
    bool isConnected() const;
    
private:
    int m_socketFd = -1;
    std::atomic<bool> m_connected{false};
    std::thread m_networkThread;
    std::vector<uint8_t> m_readBuffer;
    std::mutex m_bufferMutex;
};
```

### 4.2 连接生命周期
```
客户端启动 → 连接服务器 → 登录鉴权 → 业务操作 → 断线重连 → 退出
```

**关键状态**：
```cpp
enum class ConnectionState {
    Disconnected,     // 未连接
    Connecting,       // 正在连接
    Connected,        // 已连接（未登录）
    Authenticated,    // 已认证
    Reconnecting,     // 重连中
    Kicked            // 被踢下线
};
```

### 4.3 断线重连策略
**退避算法**：
```cpp
// 指数退避重连
int baseInterval = 1000;  // 初始1秒
int maxInterval = 30000;  // 最大30秒
int currentInterval = baseInterval;

void reconnect() {
    while (!m_connected && shouldReconnect()) {
        auto result = connectServer(m_host, m_port);
        if (result.isOk()) {
            // 重连成功，恢复登录态
            restoreSession();
            currentInterval = baseInterval;  // 重置间隔
            break;
        }
        
        // 等待后重试
        std::this_thread::sleep_for(std::chrono::milliseconds(currentInterval));
        currentInterval = std::min(currentInterval * 2, maxInterval);  // 指数增长
    }
}
```

### 4.4 心跳机制
**心跳规则**（必须与服务端一致）：
- **发送间隔**: 每10秒发送一次心跳
- **超时阈值**: 30秒内未收到响应视为超时
- **最大丢失次数**: 连续3次超时后主动断开

```cpp
void heartbeatLoop() {
    while (m_connected) {
        // 发送心跳
        sendHeartbeat();
        
        // 等待10秒
        std::this_thread::sleep_for(std::chrono::seconds(HEARTBEAT_INTERVAL));
        
        // 检查是否超时
        if (checkHeartbeatTimeout()) {
            handleConnectionLost();
            break;
        }
    }
}
```

---

## 5. 协议对接规范

### 5.1 协议一致性要求（强制）
⚠️ **客户端必须严格遵循服务端协议定义**：

| 项目 | 规范值 | 说明 |
|------|--------|------|
| 传输协议 | TCP长连接 | 固定不变 |
| 包格式 | Length(4B) + Header(48B) + Body(variable) | 固定格式 |
| 字节序 | Big-Endian (网络字节序) | 多字节字段统一 |
| Body编码 | Protobuf (proto3) | 序列化方式 |
| 包名空间 | `chatroom.protocol` | Proto包名 |
| Header长度 | 48字节 | 固定长度 |

### 5.2 消息类型枚举（与服务端完全一致）
```cpp
enum class MessageType : uint16_t {
    MSG_LOGIN = 1001,           // 用户登录认证
    MSG_LOGIN_RESP = 1002,      // 登录响应
    MSG_LOGOUT = 1003,          // 用户登出
    MSG_KICK = 1004,            // 被踢下线
    MSG_DANMAKU = 2001,         // 发送弹幕
    MSG_GIFT = 2002,            // 发送礼物
    MSG_JOIN_ROOM = 3001,       // 进入房间
    MSG_LEAVE_ROOM = 3002,      // 离开房间
    MSG_HEARTBEAT = 9001,       // 心跳
    MSG_ROOM_CREATE = 3003,     // 创建房间
    MSG_ROOM_CLOSE = 3004,      // 关闭房间
    MSG_ROOM_STATE_SYNC = 3005, // 房间状态同步
    MSG_ERROR = 5001,           // 错误返回
    MSG_ACK = 5002              // 通用确认
};
```

**⚠️ 重要**: 不允许客户端私自扩展消息类型！

### 5.3 消息收发示例
#### 发送消息（登录请求）
```cpp
Result<void> sendLoginRequest(const std::string& userName, const std::string& password) {
    // 1. 构造Protobuf Body
    chatroom::protocol::LoginBody loginBody;
    loginBody.set_user_name(userName);
    loginBody.set_password_hash(sha256(password));  // SHA-256哈希
    
    // 2. 序列化Body
    std::string bodyData = loginBody.SerializeAsString();
    
    // 3. 构造Header
    MessageHeader header;
    header.magic = MAGIC_NUMBER;
    header.version = PROTOCOL_VERSION;
    header.msgType = static_cast<uint16_t>(MessageType::MSG_LOGIN);
    header.seq = generateSeq();
    header.bodyLen = bodyData.size();
    
    // 4. 封装并发送
    return sendMessage(header, bodyData);
}
```

#### 接收消息（弹幕广播）
```cpp
void onMessageReceived(const MessageHeader& header, const std::string& bodyData) {
    switch (static_cast<MessageType>(header.msgType)) {
        case MessageType::MSG_DANMAKU: {
            chatroom::protocol::DanmakuBody danmakuBody;
            if (danmakuBody.ParseFromString(bodyData)) {
                // 更新UI显示弹幕
                onDanmakuReceived(danmakuBody);
            } else {
                logError("Failed to parse danmaku body");
            }
            break;
        }
        // ... 其他消息类型处理
    }
}
```

### 5.4 Protobuf定义文件（与服务端共享）
**必须使用服务端的.proto文件**，不要私自修改：
```protobuf
syntax = "proto3";
package chatroom.protocol;

message DanmakuBody {
    uint64 room_id = 1;
    uint64 user_id = 2;
    string user_name = 3;
    string content = 4;
    uint64 sent_at = 5;
}

message LoginRespBody {
    uint64 user_id = 1;
    bool success = 2;
    string token = 3;
    string message = 4;
    uint32 role = 5;
}

// ... 更多消息体定义
```

---

## 6. UI交互规范

### 6.1 页面结构
```
┌─────────────────────────────────────────┐
│              主窗口                      │
├──────────────┬──────────────────────────┤
│              │                          │
│   房间列表   │      房间内容区           │
│   (左侧栏)   │  ┌────────────────────┐  │
│              │  │   视频播放区        │  │
│  - 热门推荐  │  ├────────────────────┤  │
│  - 我的关注  │  │                    │  │
│  - 搜索结果  │  │   弹幕展示区        │  │
│              │  │   (滚动区域)        │  │
│              │  │                    │  │
│              │  ├────────────────────┤  │
│              │  │  输入框 | 发送按钮  │  │
│              │  └────────────────────┘  │
└──────────────┴──────────────────────────┘
```

### 6.2 交互流程
#### 登录流程
```
打开应用 → 显示登录界面 → 输入用户名/密码 → 点击登录
→ 发送MSG_LOGIN → 等待MSG_LOGIN_RESP
→ 成功: 进入主页 / 失败: 显示错误信息
```

#### 进房流程
```
选择房间 → 点击进入 → 发送MSG_JOIN_ROOM
→ 等待响应 → 加载房间信息 → 显示弹幕区域
→ 加载最近弹幕 → 开始接收实时弹幕
```

#### 发送弹幕流程
```
输入弹幕内容 → 点击发送 → 本地预检(长度/空值)
→ 封装MSG_DANMAKU → 发送到服务端
→ 等待确认 → 成功: 显示在弹幕区 / 失败: 提示错误
```

### 6.3 UI状态反馈
| 操作 | 成功状态 | 失败状态 | 进行中状态 |
|------|----------|----------|------------|
| 登录 | 跳转主页 | 显示错误信息 | Loading动画 |
| 进房 | 显示房间内容 | Toast提示 | Loading遮罩 |
| 发弹幕 | 弹幕出现在屏幕 | 错误提示 | 按钮禁用+Loading |
| 送礼 | 动画效果 | 余额不足等提示 | 礼物面板锁定 |

### 6.4 输入校验
```cpp
// 发送弹幕前的本地预检
Result<void> validateDanmakuInput(const std::string& content) {
    if (content.empty()) {
        return Result<void>::fail(ErrorCode::InvalidArgument, "弹幕内容不能为空");
    }
    
    if (content.size() > MAX_DANMAKU_LENGTH) {  // 512字符
        return Result<void>::fail(ErrorCode::InvalidArgument, 
                                  "弹幕内容过长，最多512个字符");
    }
    
    // 可选：本地敏感词预检（仅提示，不拦截）
    if (localSensitiveWordCheck(content)) {
        showWarning("内容可能包含敏感词汇");
    }
    
    return Result<void>::ok({});
}
```

---

## 7. 状态管理规范

### 7.1 全局状态对象
```cpp
class SessionManager {
public:
    // 连接状态
    ConnectionState getConnectionState() const;
    
    // 登录态
    bool isLoggedIn() const;
    uint64_t getUserId() const;
    std::string getUserName() const;
    std::string getToken() const;
    uint32_t getUserRole() const;
    
    // 当前房间
    uint64_t getCurrentRoomId() const;
    RoomState getCurrentRoomState() const;
    
    // 状态变更通知
    void addStateChangeListener(StateChangeListener* listener);
    
private:
    ConnectionState m_connectionState = ConnectionState::Disconnected;
    uint64_t m_userId = 0;
    std::string m_token;
    uint32_t m_role = 0;
    uint64_t m_currentRoomId = 0;
    RoomState m_roomState = RoomState::Closed;
    std::vector<StateChangeListener*> m_listeners;
    mutable std::mutex m_mutex;
};
```

### 7.2 状态变更通知机制
```cpp
// 观察者模式
class StateChangeListener {
public:
    virtual void onLoginSuccess(uint64_t userId, const std::string& userName) = 0;
    virtual void onLoginFailed(ErrorCode code, const std::string& message) = 0;
    virtual void onConnected() = 0;
    virtual void onDisconnected() = 0;
    virtual void onRoomJoined(uint64_t roomId) = 0;
    virtual void onRoomLeft(uint64_t roomId) = 0;
    virtual void onKicked(const std::string& reason) = 0;
};

// UI组件注册监听
class MainWindow : public StateChangeListener {
public:
    void onLoginSuccess(uint64_t userId, const std::string& userName) override {
        // 切换到主页面
        switchToMainPage();
        loadRoomList();
    }
    
    void onDisconnected() override {
        // 显示断线提示
        showDisconnectDialog();
        // 自动开始重连
        startReconnect();
    }
};
```

### 7.3 状态持久化
**本地保存的数据**（用于快速恢复）：
```cpp
struct LocalCache {
    // 用户会话
    std::string lastUserName;
    std::string lastToken;
    uint64_t lastUserId = 0;
    
    // 房间信息
    uint64_t lastRoomId = 0;
    std::vector<RoomInfo> recentRooms;  // 最近访问的房间
    
    // 弹幕历史
    std::deque<DanmakuInfo> recentDanmaku;  // 最近N条弹幕
    
    // 未发送消息
    std::queue<PendingMessage> unsentMessages;  // 待重试消息
};
```

---

## 8. 弹幕模块开发指南

### 8.1 弹幕数据结构
```cpp
struct DanmakuInfo {
    uint64_t messageId;
    uint64_t roomId;
    uint64_t userId;
    std::string userName;
    std::string content;
    std::chrono::system_clock::time_point sentAt;
    DanmakuType type;  // 普通/礼物/系统公告
};
```

### 8.2 弹幕展示策略
#### 展示容器
```cpp
class DanmakuDisplay {
public:
    // 添加弹幕到展示区
    void addDanmaku(const DanmakuInfo& danmaku);
    
    // 清空所有弹幕
    void clearAll();
    
    // 设置最大容量（防止内存溢出）
    void setMaxCapacity(size_t maxCount);
    
private:
    std::deque<DanmakuInfo> m_danmakuList;
    size_t m_maxCapacity = 1000;  // 最多保留1000条
    std::mutex m_mutex;
};
```

#### 渲染优化
```cpp
// 批量刷新，避免高频重绘
void DanmakuDisplay::addDanmaku(const DanmakuInfo& danmaku) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_danmakuList.push_back(danmaku);
    
    // 超过容量时移除旧弹幕
    while (m_danmakuList.size() > m_maxCapacity) {
        m_danmakuList.pop_front();
    }
    
    // 批量刷新UI（不是每条都立即刷新）
    scheduleRefresh();  // 16ms刷新一次（60fps）
}
```

### 8.3 乐观回显策略
```cpp
// 发送弹幕后先乐观回显，再等服务端确认
Result<void> sendDanmakuWithOptimisticUpdate(const std::string& content) {
    // 1. 本地预检
    auto validateResult = validateDanmakuInput(content);
    if (!validateResult.isOk()) {
        return validateResult;
    }
    
    // 2. 乐观回显（立即显示在界面上）
    DanmakuInfo localDanmaku;
    localDanmaku.content = content;
    localDanmaku.userId = SessionManager::getInstance().getUserId();
    localDanmaku.userName = SessionManager::getInstance().getUserName();
    localDanmaku.sentAt = std::chrono::system_clock::now();
    localDanmaku.type = DanmakuType::LocalPending;  // 待确认状态
    
    m_display.addDanmaku(localDanmaku);
    
    // 3. 异步发送到服务端
    auto result = m_protocol.sendDanmaku(content);
    
    // 4. 根据结果更新状态
    if (result.isOk()) {
        updateDanmakuStatus(localDanmaku.messageId, DanmakuType::Confirmed);
    } else {
        // 发送失败，回滚或标记为失败
        updateDanmakuStatus(localDanmaku.messageId, DanmakuType::Failed);
        showError(result.message());
        
        // 保存到未发送队列，稍后重试
        m_unsentQueue.enqueue({content, 3});  // 最多重试3次
    }
    
    return result;
}
```

### 8.4 弹幕失败重试
```cpp
// 未发送消息队列
class UnsentMessageQueue {
public:
    void enqueue(const PendingMessage& msg);
    void retryAll();  // 网络恢复后批量重试
    
private:
    std::queue<PendingMessage> m_queue;
    std::mutex m_mutex;
};

struct PendingMessage {
    std::string content;
    int remainingRetries;  // 剩余重试次数
    std::chrono::system_clock::time_point firstAttemptTime;
};
```

---

## 9. 房间模块开发指南

### 9.1 房间数据结构
```cpp
struct RoomInfo {
    uint64_t roomId;
    std::string roomName;
    uint64_t hostUserId;
    std::string hostName;
    std::string coverUrl;
    RoomState state;           // CREATED/READY/LIVE/OFFLINE/CLOSED
    int onlineCount;           // 在线人数
    uint64_t danmakuCount;     // 弹幕总数
    std::chrono::system_clock::time_point createdAt;
};
```

### 9.2 房间列表管理
```cpp
class RoomListManager {
public:
    // 分页查询
    Result<std::vector<RoomInfo>> queryRoomList(int page, int pageSize, 
                                                  RoomFilter filter);
    
    // 缓存最近浏览的房间
    void addToRecentRooms(uint64_t roomId);
    std::vector<RoomInfo> getRecentRooms(size_t maxCount = 20);
    
    // 刷新单个房间信息
    Result<RoomInfo> refreshRoomInfo(uint64_t roomId);
    
private:
    std::map<uint64_t, RoomInfo> m_roomCache;  // 房间缓存
    std::deque<uint64_t> m_recentRoomIds;      // 最近访问
    mutable std::shared_mutex m_rwLock;        // 读写锁
};
```

### 9.3 房间状态同步
```cpp
// 监听服务端推送的房间状态变更
void onRoomStateSyncReceived(const MessageHeader& header, const std::string& bodyData) {
    chatroom::protocol::RoomStateBody stateBody;
    if (!stateBody.ParseFromString(bodyData)) {
        return;
    }
    
    uint64_t roomId = stateBody.room_id();
    RoomState newState = static_cast<RoomState>(stateBody.state());
    
    // 更新本地缓存
    updateRoomState(roomId, newState);
    
    // 如果当前正在该房间，更新UI
    if (SessionManager::getInstance().getCurrentRoomId() == roomId) {
        onCurrentRoomStateChanged(newState);
        
        // 房间关闭时自动离开
        if (newState == RoomState::Closed) {
            leaveRoom(roomId);
            showRoomClosedDialog("房间已关闭");
        }
    }
}
```

### 9.4 在线人数展示
```cpp
// 在线人数以服务端同步值为准，本地仅做展示缓存
void updateOnlineCount(uint64_t roomId, int count) {
    // 更新缓存
    m_roomCache[roomId].onlineCount = count;
    
    // 更新UI（节流，避免频繁刷新）
    if (shouldRefreshOnlineCount()) {
        refreshOnlineCountDisplay(count);
    }
}

// 节流：每3秒最多刷新一次在线人数
bool shouldRefreshOnlineCount() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = now - m_lastOnlineCountRefresh;
    return elapsed >= std::chrono::seconds(3);
}
```

---

## 10. 本地缓存策略

### 10.1 缓存对象清单
| 缓存对象 | 数据内容 | 生命周期 | 持久化 |
|----------|----------|----------|--------|
| 会话缓存 | userId/token/role/userName | 登录期间 | 否 |
| 房间列表缓存 | 最近查询的房间列表 | 应用运行期间 | 可选 |
| 房间快照缓存 | 当前房间的详细信息 | 在房期间 | 否 |
| 最近弹幕缓存 | 最近N条弹幕记录 | 在房期间 | 否 |
| 未发送队列 | 网络异常时的待重试消息 | 直到重试完成或放弃 | 否 |
| 用户偏好设置 | UI设置、字体大小等 | 长期 | 是（本地文件）|

### 10.2 缓存失效策略
```cpp
class CacheManager {
public:
    // 登录切换时清理上一用户残留缓存
    void clearUserSpecificCache() {
        clearSession();
        clearUnsentQueue();
        clearRecentDanmaku();
        clearCurrentRoomSnapshot();
    }
    
    // 应用退出时清理临时缓存
    void clearTempCacheOnExit() {
        clearSession();
        clearCurrentRoomSnapshot();
        // 保留用户偏好设置和最近访问房间
    }
    
    // 定期清理过期数据
    void cleanupExpiredData() {
        cleanExpiredRecentRooms(7 * 24 * 3600);  // 7天过期
        cleanOldUnsentMessages(3600);  // 1小时过期
    }
};
```

### 10.3 本地存储格式（可选）
```json
{
  "user_preferences": {
    "font_size": 14,
    "danmaku_opacity": 0.8,
    "auto_reconnect": true,
    "last_login_user": "user123"
  },
  "recent_rooms": [
    {"room_id": 1001, "visited_at": "2026-05-15T10:30:00"},
    {"room_id": 1002, "visited_at": "2026-05-14T15:20:00"}
  ]
}
```

---

## 11. 性能优化指南

### 11.1 优化目标
| 指标 | 目标值 | 说明 |
|------|--------|------|
| 首屏加载时间 | <2秒 | 从启动到显示登录页 |
| 房间列表加载 | <500ms | 分页数据加载 |
| 弹幕显示延迟 | <100ms | 从接收到显示 |
| UI帧率 | ≥60fps | 主线程流畅度 |
| 内存占用 | <500MB | 正常使用场景 |

### 11.2 优化方向
#### 1. 房间列表虚拟化
```cpp
// 只渲染可见区域的列表项，避免一次性创建过多UI元素
class VirtualListView : public QAbstractListModel {
    // 只维护可视区域内的item
    // 滚动时动态加载和卸载
};
```

#### 2. 弹幕展示批量刷新
```cpp
// 避免每条弹幕都触发UI重绘
// 改为批量收集，定时刷新（如16ms间隔）
void batchRefreshDanmaku() {
    std::lock_guard<std::mutex> lock(m_pendingMutex);
    if (!m_pendingDanmaku.empty()) {
        // 批量添加到显示区
        for (const auto& danmaku : m_pendingDanmaku) {
            m_display.addDanmakuInternal(danmaku);
        }
        m_pendingDanmaku.clear();
        
        // 触发一次UI刷新
        triggerUiRefresh();
    }
}
```

#### 3. 网络断线重连优化
```cpp
// 使用指数退避策略，避免频繁重连浪费资源
// 首次1s → 2s → 4s → 8s → ... → 最大30s
```

#### 4. 本地缓存容量控制
```cpp
// 固定容量 + 过期淘汰
template<typename T>
class BoundedCache {
public:
    void put(const Key& key, const T& value);
    std::optional<T> get(const Key& key);
    
private:
    std::map<Key, CacheEntry<T>> m_cache;
    size_t m_maxSize;
    std::chrono::seconds m_ttl;
};
```

### 11.3 禁止的优化行为
❌ **不要为了性能牺牲正确性**：
- 不能跳过协议校验
- 不能绕过错误处理
- 不能使用未经验证的优化技巧
- 不能破坏与服务端的协议一致性

---

## 12. 测试规范

### 12.1 测试范围
| 测试类型 | 覆盖范围 | 工具 |
|----------|----------|------|
| 单元测试 | 协议编解码、状态机、缓存、重试策略 | Google Test |
| 集成测试 | 连接服务端、登录、进房、发弹幕、退房 | 自研测试脚本 |
| 联调测试 | 与服务端一起验证消息流转和错误处理 | 手动+自动化 |
| UI测试 | 界面交互、状态切换、异常提示 | Qt Test / 框架自带 |

### 12.2 必须覆盖的核心场景
✅ **主链路（必须稳定）**:
1. 登录成功并切换到已认证状态
2. 进入房间并加载房间信息
3. 发送弹幕并收到服务端确认
4. 接收其他用户的弹幕广播
5. 断线后自动重连成功
6. 离开房间并清理资源

✅ **异常场景（必须覆盖）**:
1. 网络断开时的优雅降级
2. 登录失败（密码错误/网络问题）
3. 敏感词命中后的提示
4. 限流触发后的等待提示
5. 房间关闭后的强制退出
6. 被踢下线的处理

### 12.3 测试用例示例
```cpp
TEST(ProtocolTest, EncodeDecodeLoginMessage) {
    // 构造登录消息
    LoginBody loginBody;
    loginBody.set_user_name("testuser");
    loginBody.set_password_hash("abcdef1234567890");
    
    // 编码
    std::string encoded = loginBody.SerializeAsString();
    ASSERT_FALSE(encoded.empty());
    
    // 解码
    LoginBody decoded;
    ASSERT_TRUE(decoded.ParseFromString(encoded));
    EXPECT_EQ(decoded.user_name(), "testuser");
    EXPECT_EQ(decoded.password_hash(), "abcdef1234567890");
}

TEST(StateMachineTest, ConnectionStateTransition) {
    SessionManager session;
    
    // 初始状态：Disconnected
    EXPECT_EQ(session.getConnectionState(), ConnectionState::Disconnected);
    
    // 连接成功
    session.onConnected();
    EXPECT_EQ(session.getConnectionState(), ConnectionState::Connected);
    
    // 登录成功
    session.onLoginSuccess(123, "testuser");
    EXPECT_EQ(session.getConnectionState(), ConnectionState::Authenticated);
    EXPECT_TRUE(session.isLoggedIn());
    EXPECT_EQ(session.getUserId(), 123);
    
    // 断开连接
    session.onDisconnected();
    EXPECT_EQ(session.getConnectionState(), ConnectionState::Disconnected);
    EXPECT_FALSE(session.isLoggedIn());
}
```

---

## 13. 常见问题FAQ

### Q1: 如何保证客户端与服务端协议一致？
**A**: 
- 使用服务端提供的`.proto`文件，不要私自修改
- 共享`MessageType`枚举定义
- 定期与服务端联调验证
- 发现不一致时以服务端为准

### Q2: 弹幕发送失败如何处理？
**A**: 
1. 显示明确的错误提示给用户
2. 将失败消息加入未发送队列
3. 网络恢复后自动重试（最多3次）
4. 超过重试次数后提示用户手动重发

### Q3: 大量弹幕时UI卡顿怎么办？
**A**: 
1. 使用虚拟列表只渲染可见区域
2. 批量收集弹幕，定时刷新（16ms间隔）
3. 限制最大显示数量（如1000条）
4. 超出容量的旧弹幕自动移除

### Q4: 如何处理敏感词？
**A**: 
- **客户端**: 可选加载轻量词库做本地提示（仅提示，不拦截）
- **服务端**: 最终过滤由服务端AC自动机完成
- 客户端预检结果不得替代服务端结果
- 预检功能必须可关闭

### Q5: 断线重连后如何恢复状态？
**A**: 
1. 重新建立TCP连接
2. 重新发送登录请求（使用保存的token或重新登录）
3. 如果之前在房间中，重新发送进房请求
4. 服务端返回最近弹幕用于恢复展示
5. 处理未发送队列中的消息

### Q6: 在线人数不准怎么办？
**A**: 
- 在线人数以服务端同步值为准
- 本地仅做展示缓存，不作为权威数据
- 接收到服务端推送后立即更新
- 可以显示"约XXX人在线"降低精确性期望

---

## 📚 附录

### A. 开发检查清单
- [ ] 代码符合命名规范
- [ ] 所有外部输入都有校验
- [ ] 错误处理使用Result封装
- [ ] 日志输出符合格式要求
- [ ] 协议消息与服务端完全一致
- [ ] UI操作不阻塞主线程
- [ ] 状态变更通过观察者通知
- [ ] 本地缓存有失效策略
- [ ] 断线重连有退避策略
- [ ] 单元测试覆盖核心路径

### B. 关键配置项
```jsonc
{
  "server": {
    "host": "127.0.0.1",
    "port": 8900
  },
  "reconnect": {
    "base_interval_ms": 1000,
    "max_interval_ms": 30000,
    "max_attempts": 10
  },
  "heartbeat": {
    "interval_sec": 10,
    "timeout_sec": 30,
    "max_miss_count": 3
  },
  "ui": {
    "danmaku_max_count": 1000,
    "refresh_interval_ms": 16,
    "list_page_size": 20
  },
  "cache": {
    "recent_rooms_max": 20,
    "recent_danmaku_max": 200,
    "unsent_message_ttl_sec": 3600
  }
}
```

### C. 与服务端协作接口
| 客户端调用 | 服务端接口 | 说明 |
|------------|------------|------|
| `connectServer()` | TCP连接 | 建立长连接 |
| `login()` | MSG_LOGIN → MSG_LOGIN_RESP | 用户认证 |
| `joinRoom()` | MSG_JOIN_ROOM | 进入房间 |
| `leaveRoom()` | MSG_LEAVE_ROOM | 离开房间 |
| `sendDanmaku()` | MSG_DANMAKU | 发送弹幕 |
| `sendGift()` | MSG_GIFT | 发送礼物 |
| 心跳 | MSG_HEARTBEAT | 保活连接 |

---

**📌 重要提醒**: 客户端必须优先服从服务端协议和状态机。不允许客户端私自定义消息格式或字段含义。所有新增能力都要在服务端文档找到明确映射。
