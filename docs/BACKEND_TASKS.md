# 直播弹幕系统 — 后端/服务端开发任务清单

> **版本**: v1.0
> **创建日期**: 2026-05-17
> **适用角色**: 后端工程师、C++开发工程师、系统架构师
> **预估总工期**: 6-8周（1人全职）或 3-4周（2人并行）
> **依赖文档**: BACKEND_RULES.md, 00~12号设计文档

---

## 📋 任务概览

| 阶段 | 任务数 | 预估工时 | 优先级 | 并行度 |
|------|--------|----------|--------|--------|
| Phase 0: 项目基础设施 | 6 | 2天 | P0 | - |
| Phase 1: 核心数据结构 | 5 | 3天 | P0 | 可与Phase 0并行 |
| Phase 2: 数据层实现 | 8 | 4天 | P0 | 依赖Phase 0 |
| Phase 3: 协议层实现 | 6 | 3天 | P0 | 依赖Phase 0 |
| Phase 4: 网络层实现 | 9 | 5天 | P0 | 依赖Phase 2, 3 |
| Phase 5: 业务层-基础服务 | 7 | 4天 | P0 | 依赖Phase 4 |
| Phase 6: 业务层-弹幕模块 | 6 | 3天 | P0 | 依赖Phase 5 |
| Phase 7: 业务层-房间模块 | 6 | 3天 | P0 | 依赖Phase 5 |
| Phase 8: 业务层-敏感词过滤 | 5 | 3天 | P1 | 可与Phase 6,7并行 |
| Phase 9: 性能优化 | 6 | 4天 | P1 | 依赖Phase 6,7,8 |
| Phase 10: 测试与质量保障 | 8 | 5天 | P0 | 贯穿全程 |
| **合计** | **72** | **43天** | - | - |

---

## 🏗️ Phase 0: 项目基础设施 (P0 - 2天)

### Task 0.1: CMake构建系统配置
- **优先级**: P0
- **预估**: 4小时
- **依赖**: 无
- **交付物**:
  - [ ] 创建根目录 `CMakeLists.txt`（全局选项、版本号、C++17标准）
  - [ ] 配置vcpkg工具链文件集成
  - [ ] 设置编译选项：Debug(-g) / Release(-O2 -DNDEBUG)
  - [ ] 配置第三方库查找：spdlog, fmt, protobuf, gtest, yaml-cpp, nlohmann/json, hiredis, mysqlclient
  - [ ] 创建子目录CMakeLists.txt模板
- **验收标准**: `cmake -B build && cmake --build build` 成功

### Task 0.2: 目录结构与代码框架
- **优先级**: P0
- **预估**: 2小时
- **依赖**: Task 0.1
- **交付物**:
  - [ ] 按照架构规范创建完整目录结构
  - [ ] 创建所有公共头文件骨架（空类/接口定义）
  - [ ] 创建命名空间定义 `namespace chatroom { namespace net {} ... }`
  - [ ] 创建主程序入口 `src/main.cpp`（打印启动日志）
- **参考文档**: 01_项目架构设计.md 第5节

### Task 0.3: 日志系统集成
- **优先级**: P0
- **预估**: 2小时
- **依赖**: Task 0.1
- **交付物**:
  - [ ] 封装spdlog为项目统一日志接口 (`include/common/logging.h`)
  - [ ] 实现日志格式：`[时间][级别][模块][线程ID][fd/roomId][消息]`
  - [ ] 支持控制台 + 文件双输出
  - [ ] 文件按天分割 / 按大小(100MB)轮转
  - [ ] 各模块日志分类输出（net/protocol/service/data）
- **验收标准**: 日志格式符合00_开发规范.md第6节规范

### Task 0.4: 配置文件加载模块
- **优先级**: P0
- **预估**: 3小时
- **依赖**: Task 0.1
- **交付物**:
  - [ ] 实现 `ConfigManager` 类（读取JSON/YAML配置）
  - [ ] 定义所有配置项结构体（ServerConfig, MysqlConfig, RedisConfig等）
  - [ ] 配置项默认值 + 校验逻辑
  - [ ] 配置文件示例 `config/app.json`
  - [ ] 环境变量覆盖支持（可选）
- **参考文档**: 09_构建与部署.md 第6节

### Task 0.5: 错误码与Result封装
- **优先级**: P0
- **预估**: 2小时
- **依赖**: 无
- **交付物**:
  - [ ] 定义ErrorCode枚举（Success/InvalidArgument/ProtocolError等）
  - [ ] 实现Result<T>模板类（ok/fail/isOk/value/code/message）
  - [ ] 定义VoidResult别名
  - [ ] 单元测试：Result的各种用法
- **参考文档**: 00_开发规范.md 第5节, 05_接口统一标准.md 第2节

### Task 0.6: 回调类型定义
- **优先级**: P0
- **预估**: 1小时
- **依赖**: Task 0.5
- **交付物**:
  - [ ] 定义所有回调函数类型别名（ConnectCallback/MessageCallback等）
  - [ ] 统一回调参数签名
  - [ ] 头文件 `include/common/callbacks.h`
- **参考文档**: 05_接口统一标准.md 第3节

---

## 🔧 Phase 1: 核心数据结构 (P0 - 3天)

### Task 1.1: 线程池实现
- **优先级**: P0
- **预估**: 6小时
- **依赖**: Task 0.5
- **交付物**:
  - [ ] 实现 `ThreadPool` 类（互斥锁+条件变量任务队列）
  - [ ] 任务结构体Task{callback, taskId, roomId, userId}
  - [ ] submit()提交任务，返回taskId
  - [ ] workerLoop()工作线程主循环
  - [ ] shutdown()/shutdownNow()优雅关闭
  - [ ] 默认线程数 = CPU核心数×2
- **测试用例**:
  - 单任务提交执行
  - 大量并发任务压力测试
  - shutdown时未完成任务处理
- **参考文档**: 04_核心数据结构设计.md 第2节

### Task 1.2: 定时器（时间轮）实现
- **优先级**: P0
- **预估**: 6小时
- **依赖**: 无
- **交付物**:
  - [ ] 实现 `TimerWheel` 类（1024槽位，100ms tick间隔）
  - [ ] addTimer(timeoutMs, callback)添加定时任务
  - [ ] cancelTimer(taskId)取消定时任务
  - [ ] tick()时间推进函数
  - [ ] 定时线程独立运行
- **测试用例**:
  - 单次定时触发
  - 多个定时任务并发
  - 取消定时任务验证
- **参考文档**: 04_核心数据结构设计.md 第6节

### Task 1.3: 连接管理器数据结构
- **优先级**: P0
- **预估**: 4小时
- **依赖**: Task 0.5
- **交付物**:
  - [ ] Connection对象定义（fd/state/userId/token/roomId/heartbeatTime/readBuffer）
  - [ ] ConnState枚举（UNAUTHED/AUTHED/KICKED）
  - [ ] ConnectionManager骨架（addConnection/removeConnection/getConnection/broadcast）
  - [ ] 使用std::unordered_map<int, shared_ptr<Connection>>存储
  - [ ] 使用shared_mutex保护（读多写少场景）
- **参考文档**: 04_核心数据结构设计.md 第5节

### Task 1.4: 消息队列设计
- **优先级**: P0
- **预估**: 3小时
- **依赖**: Task 1.1
- **交付物**:
  - [ ] 线程安全消息队列（基于mutex+condition_variable）
  - [ ] push(Task)/pop()操作
  - [ ] 非阻塞tryPop()
  - [ ] 队列容量上限（65536）
  - [ ] 队列满时的处理策略（丢弃或阻塞）
- **参考文档**: 04_核心数据结构设计.md 第3节

### Task 1.5: 工具函数库
- **优先级**: P1
- **预估**: 3小时
- **依赖**: 无
- **交付物**:
  - [ ] 时间戳生成工具（毫秒级）
  - [ ] SHA-256哈希计算工具
  - [ ] UUID/随机Token生成工具
  - [ ] 字符串处理工具（trim/split等）
  - [ ] 头文件 `include/util/utils.h`

---

## 💾 Phase 2: 数据层实现 (P0 - 4天)

### Task 2.1: MySQL连接池实现
- **优先级**: P0
- **预估**: 6小时
- **依赖**: Task 0.4
- **交付物**:
  - [ ] 实现 `MySqlPool` 类（连接池管理）
  - [ ] 配置参数：min=4, max=32, idleTimeout=60s, connectTimeout=3s
  - [ ] getConnection()获取连接（带超时）
  - [ ] returnConnection()归还连接
  - [ ] 连接健康检查（ping检测）
  - [ ] 空闲连接回收机制
- **测试用例**:
  - 连接池初始化和销毁
  - 并发获取归还连接
  - 连接超时处理
- **参考文档**: 03_数据库设计.md 第5节

### Task 2.2: Redis连接池实现
- **优先级**: P0
- **预估**: 4小时
- **依赖**: Task 0.4
- **交付物**:
  - [ ] 实现 `RedisPool` 类
  - [ ] 配置参数：min=2, max=16, connectTimeout=2s, retryCount=3
  - [ ] getContext()获取Redis上下文
  - [ ] 断线重连机制
- **测试用例**: 基本的SET/GET/DEL操作验证

### Task 2.3: Repository接口定义
- **优先级**: P0
- **预估**: 2小时
- **依赖**: Task 2.1, 2.2
- **交付物**:
  - [ ] 定义DataRepository纯虚接口类
  - [ ] 用户相关接口：queryUserById/queryUserByName/updateUserToken
  - [ ] 房间相关接口：createRoom/updateRoomState/queryRoomList
  - [ ] 成员相关接口：addRoomMember/updateRoomMemberLeaveTime
  - [ ] 弹幕相关接口：saveDanmaku/queryRecentDanmaku
  - [ ] 礼物相关接口：saveGift
- **参考文档**: 03_数据库设计.md 第2节

### Task 2.4: UserRepository实现
- **优先级**: P0
- **预估**: 3小时
- **依赖**: Task 2.1, 2.3
- **交付物**:
  - [ ] MysqlRepository继承DataRepository
  - [ ] 实现用户查询SQL（参数化查询防注入）
  - [ ] 实现用户更新SQL
  - [ ] 结果集映射到UserInfo结构体
- **SQL语句**:
```sql
SELECT id, user_name, password_hash, avatar_url, role, status 
FROM users WHERE user_name = ? LIMIT 1;

UPDATE users SET token = ?, updated_at = NOW() WHERE id = ?;
```

### Task 2.5: RoomRepository实现
- **优先级**: P0
- **预估**: 4小时
- **依赖**: Task 2.1, 2.3
- **交付物**:
  - [ ] 房间CRUD SQL实现
  - [ ] 分页查询SQL（LIMIT/OFFSET）
  - [ ] 在线数批量更新SQL
  - [ ] 房间状态校验
- **SQL语句**:
```sql
INSERT INTO rooms (room_name, host_user_id, cover_url, room_state) VALUES (?, ?, ?, ?);

UPDATE rooms SET room_state = ?, updated_at = NOW() WHERE id = ? AND room_state != ?;

SELECT * FROM rooms WHERE room_state = ? ORDER BY online_count DESC LIMIT ? OFFSET ?;
```

### Task 2.6: RoomMemberRepository实现
- **优先级**: P0
- **预估**: 3小时
- **依赖**: Task 2.1, 2.3
- **交付物**:
  - [ ] INSERT ON DUPLICATE KEY UPDATE实现（同用户同房间复用记录）
  - [ ] 更新leave_time
  - [ ] 查询房间成员列表
- **关键SQL** (必须使用):
```sql
INSERT INTO room_members (room_id, user_id, member_role, join_time, leave_time)
VALUES (?, ?, ?, NOW(), NULL)
ON DUPLICATE KEY UPDATE
    member_role = VALUES(member_role),
    join_time = VALUES(join_time),
    leave_time = NULL;
```
- **参考文档**: 03_数据库设计.md 第2.3.1节

### Task 2.7: DanmakuRepository实现
- **优先级**: P0
- **预估**: 2小时
- **依赖**: Task 2.1, 2.3
- **交付物**:
  - [ ] 弹幕插入SQL
  - [ ] 最近弹幕查询SQL（按时间倒序）
  - [ ] content_status字段使用（0=正常, 1=已过滤）

### Task 2.8: RedisHelper工具类实现
- **优先级**: P0
- **预估**: 6小时
- **依赖**: Task 2.2
- **交付物**:
  - [ ] 房间信息缓存：HGET/HSET room:info:{roomId}
  - [ ] 在线用户集合：SADD/SREM/SMEMBERS room:online:{roomId}
  - [ ] 最近弹幕列表：LPUSH/LRANGE room:recent_danmaku:{roomId}
  - [ ] 限流计数器：INCR/EXPIRE limit:danmaku:user:{userId}
  - [ ] 排行榜：ZINCRBY/ZREVRANGEBYSCORE rank:room:gift
  - [ ] 会话Token：SET session:{token} EX 86400
- **参考文档**: 03_数据库设计.md 第4节

---

## 📦 Phase 3: 协议层实现 (P0 - 3天)

### Task 3.1: Protobuf定义文件编写
- **优先级**: P0
- **预估**: 2小时
- **依赖**: 无
- **交付物**:
  - [ ] proto/common.proto（ErrorBody/AckBody）
  - [ ] proto/auth.proto（LoginBody/LoginRespBody/LogoutBody/KickBody）
  - [ ] proto/room.proto（RoomStateBody/JoinRoomBody/LeaveRoomBody）
  - [ ] proto/danmaku.proto（DanmakuBody）
  - [ ] proto/gift.proto（GiftBody）
  - [ ] 所有包名统一为 `chatroom.protocol`
- **验收标准**: protoc编译通过，生成C++代码可用

### Task 3.2: MessageHeader结构体
- **优先级**: P0
- **预估**: 2小时
- **依赖**: 无
- **交付物**:
  - [ ] MessageHeader结构体定义（magic/version/headerLen/msgType/flags/seq/roomId/userId/timestamp/bodyLen）
  - [ ] 序列化方法serialize()（Big-Endian字节序转换）
  - [ ] 反序列化方法deserialize()
  - [ ] isValid()合法性校验
  - [ ] 常量定义：MAGIC_NUMBER=0x48415443, PROTOCOL_VERSION=1, HEADER_SIZE=48
- **参考文档**: 02_网络协议设计.md 第3节

### Task 3.3: MessageType枚举
- **优先级**: P0
- **预估**: 1小时
- **依赖**: 无
- **交付物**:
  - [ ] enum class MessageType : uint16_t
  - [ ] 所有消息类型常量（MSG_LOGIN=1001到MSG_ERROR=5001）
  - [ ] isValidMessageType()工具函数
  - [ ] getMessageTypeName()工具函数（用于日志）
- **参考文档**: 02_网络协议设计.md 第4节

### Task 3.4: MessageCodec编解码器
- **优先级**: P0
- **预估**: 6小时
- **依赖**: Task 3.1, 3.2, 3.3
- **交付物**:
  - [ ] encode(header, protobufBody) → 完整数据包字符串
    - 计算bodyLen → 构造header序列化 → 拼接length(4B)+header(48B)+body
    - Length字段使用uint32_t Big-Endian
  - [ ] decode(packetData) → Message{header, body}
    - 解析Length → 提取Header+Body → 反序列化Protobuf
    - 校验magic/version/bodyLen一致性
  - [ ] decodeHeaderOnly(packetData) → MessageHeader（快速路由用）
- **测试用例**:
  - 编码解码Round-trip测试
  - 各种MessageType的编解码
  - 边界条件（最小包/最大包/空body）
- **参考文档**: 02_网络协议设计.md 第2节

### Task 3.5: 消息分发器
- **优先级**: P0
- **预估**: 3小时
- **依赖**: Task 3.4
- **交付物**:
  - [ ] MessageDispatcher类
  - [ ] registerHandler(msgType, handler)注册处理器
  - [ ] dispatch(fd, message)根据msgType路由到对应handler
  - [ ] 未注册消息类型的默认处理（返回MSG_ERROR）
- **验收标准**: 能正确将不同类型消息路由到对应处理函数

### Task 3.6: 协议层单元测试
- **优先级**: P0
- **预估**: 3小时
- **依赖**: Task 3.4, 3.5
- **交付物**:
  - [ ] MessageCodec编码解码测试（≥15个用例）
  - [ ] Header字段边界值测试
  - [ ] Protobuf序列化反序列化测试
  - [ ] 消息分发正确性测试
  - [ ] 非法包检测测试
- **覆盖率目标**: ≥90%

---

## 🌐 Phase 4: 网络层实现 (P0 - 5天)

### Task 4.1: EpollServer基础框架
- **优先级**: P0
- **预估**: 6小时
- **依赖**: Phase 0完成
- **交付物**:
  - [ ] EpollServer类（epoll_create/epoll_wait事件循环）
  - [ ] init(host, port, backlog)初始化监听socket
  - [ ] start()/stop()启停控制
  - [ ] eventLoop()主循环（处理EPOLLIN/EPOLLOUT/EPOLLERR/EPOLLHUP）
  - [ ] MAX_EVENTS=1024
- **测试用例**:
  - 启动服务器并监听端口
  - telnet连接测试
  - 正常关闭服务器
- **参考文档**: 01_项目架构设计.md 第3节

### Task 4.2: 连接建立与断开处理
- **优先级**: P0
- **预估**: 4小时
- **依赖**: Task 4.1, 1.3
- **交付物**:
  - [ ] handleAccept()接受新连接
  - [ ] 创建Connection对象，初始状态UNAUTHED
  - [ ] 注册到ConnectionManager
  - [ ] 设置非阻塞IO
  - [ ] 加入epoll监听EPOLLIN事件
  - [ ] handleDisconnect()清理连接资源
  - [ ] 触发DisconnectCallback回调
- **验收标准**: 能接受客户端连接并维护连接状态

### Task 4.3: 数据接收与粘包拆包
- **优先级**: P0
- **预估**: 6小时
- **依赖**: Task 4.2, 3.4
- **交付物**:
  - [ ] handleRead(fd)读取socket数据到Connection.readBuffer
  - [ ] processReadData(conn)解析缓冲区中的完整包
  - [ ] 粘包处理：一次读取多个完整包
  - [ ] 拆包处理：半包缓存等待下次拼接
  - [ ] 包长度校验：非法长度直接断开连接
  - [ ] 缓冲区上限检查：超过上限视为异常连接
- **测试用例**:
  - 正常单包接收
  - 多包粘连一起接收
  - 大包分多次接收
  - 超大包拒绝
- **参考文档**: 02_网络协议设计.md 第6节

### Task 4.4: 数据发送实现
- **优先级**: P0
- **预估**: 3小时
- **依赖**: Task 4.2
- **交付物**:
  - [ ] sendTo(fd, data)发送数据
  - [ ] 处理EAGAIN（缓冲区满）情况
  - [ ] 使用写缓冲区暂存未发送完的数据
  - [ ] 注册EPOLLOUT事件继续发送
- **验收标准**: 能可靠发送任意长度数据

### Task 4.5: 广播功能实现
- **优先级**: P0
- **预估**: 4小时
- **依赖**: Task 4.4, 1.3
- **交付物**:
  - [ ] broadcast(roomId, message)向房间内所有在线用户广播
  - [ ] 从RoomService或Redis获取在线用户fd列表
  - [ ] 遍历fd列表逐个调用sendTo()
  - [ ] 广播时跳过已断开的连接
  - [ ] 广播失败统计和日志
- **性能要求**: 万级在线用户广播延迟<50ms

### Task 4.6: 登录鉴权流程
- **优先级**: P0
- **预估**: 6小时
- **依赖**: Task 4.3, Phase 2
- **交付物**:
  - [ ] 处理MSG_LOGIN请求
  - [ ] 从LoginBody提取userName/password_hash
  - [ ] 查询数据库验证用户
  - [ ] 生成Token（UUID或随机32字节hex）
  - [ ] 写入Redis会话：session:{token} → {userId, userName, role, expireAt}
  - [ ] 绑定Connection.userId/token/role
  - [ ] 更新Connection.state为AUTHED
  - [ ] 维护全局映射g_fdToUser/g_userToFd
  - [ ] 返回MSG_LOGIN_RESP（成功/失败+原因）
  - [ ] 密码哈希算法：SHA-256
- **测试用例**:
  - 正确账号密码登录成功
  - 错误密码登录失败
  - 不存在账号登录失败
  - 已登录连接重复登录处理
- **参考文档**: 02_网络协议设计.md 第10节

### Task 4.7: 心跳处理机制
- **优先级**: P0
- **预估**: 4小时
- **依赖**: Task 4.3, 1.2
- **交付物**:
  - [ ] 处理MSG_HEARTBEAT请求
  - [ ] 更新Connection.lastHeartbeatTime
  - [ ] 心跳包不进入工作线程池（只做时间戳刷新）
  - [ ] 定时器线程定期检查心跳超时（30秒阈值）
  - [ ] 超时连接主动断开并清理资源
  - [ ] 连续丢失心跳计数（maxMissCount=3）
- **验收标准**: 能准确检测死连接并及时清理

### Task 4.8: 未鉴权连接处理
- **优先级**: P0
- **预估**: 2小时
- **依赖**: Task 4.6
- **交付物**:
  - [ ] UNAUTHED状态连接只能发送MSG_LOGIN和MSG_HEARTBEAT
  - [ ] 其他消息类型返回MSG_ERROR（PermissionDenied）
  - [ ] 30秒未完成登录则主动断开
- **安全性**: 防止未认证连接进行业务操作

### Task 4.9: 异常断线清理流程
- **优先级**: P0
- **预估**: 3小时
- **依赖**: Task 4.7
- **交付物**:
  - [ ] 从ConnectionManager移除fd→Connection映射
  - [ ] 如果userId不为0，清理Redis在线状态
  - [ ] 删除session Token和userFd映射
  - [ ] 如果在房间中，异步更新房间在线人数
  - [ ] 广播用户离开消息给房间内其他用户
  - [ ] 关闭fd并释放Connection对象
  - [ ] 大量同时断线时分批清理（避免瞬间Redis/MySQL压力过大）
- **参考文档**: 07_房间模块设计.md 第9节

---

## ⚙️ Phase 5: 业务层-基础服务 (P0 - 4天)

### Task 5.1: 服务层基类与依赖注入
- **优先级**: P0
- **预估**: 2小时
- **依赖**: Phase 2, 3, 4完成
- **交付物**:
  - [ ] BaseService基类（可选）
  - [ ] 服务间依赖关系梳理
  - [ ] DanmakuService依赖：RoomService, FilterService, DataRepository
  - [ ] RoomService依赖：DataRepository, ConnectionManager
  - [ ] FilterService独立（仅依赖词库文件）
- **验收标准**: 服务实例能正常创建和组装

### Task 5.2: 登录服务完善
- **优先级**: P0
- **预估**: 3小时
- **依赖**: Task 4.6
- **交付物**:
  - [ ] LoginService类封装登录逻辑
  - [ ] Token生成和管理
  - [ ] 登出处理（MSG_LOGOUT）：解绑fd→userId，清理session
  - [ ] 踢人功能（MSG_KICK）：强制断开目标用户连接
  - [ ] 权限校验：只有主播/管理员可踢人
- **测试用例**: 登录/登出/踢人全流程

### Task 5.3: 主程序入口整合
- **优先级**: P0
- **预估**: 4小时
- **依赖**: Phase 4基本完成
- **交付物**:
  - [ ] src/main.cpp实现完整启动顺序：
    1. 加载配置
    2. 初始化日志
    3. 初始化MySQL连接池
    4. 初始化Redis连接池
    5. 初始化Repository
    6. 初始化Service层
    7. 初始化线程池
    8. 初始化定时器
    9. 创建EpollServer并绑定端口
    10. 启动工作线程和定时线程
    11. 进入eventLoop
  - [ ] 信号处理（SIGTERM/SIGINT优雅关闭）
  - [ ] 启动日志输出
- **验收标准**: 服务器能正常启动并接受连接

### Task 5.4: 消息处理主流程串联
- **优先级**: P0
- **预估**: 4小时
- **依赖**: Task 5.3
- **交付物**:
  - [ ] epoll收到数据 → 读取 → 解析完整包 → 投递到工作线程
  - [ ] 工作线程 → 根据msgType分发到对应Service
  - [ ] Service处理业务逻辑 → 调用DataLayer持久化
  - [ ] Service构造响应 → 通过ConnectionManager发送回客户端
  - [ ] 整条链路跑通：连接 → 登录 → 发消息 → 收响应
- **里程碑**: **M1: 主链路跑通**

### Task 5.5: 错误处理与异常捕获
- **优先级**: P0
- **预估**: 3小时
- **依赖**: Task 5.4
- **交付物**:
  - [ ] 所有可能出错的地方都有Result返回
  - [ ] Protobuf解析异常catch并返回ProtocolError
  - [ ] 数据库操作异常catch并返回DatabaseError
  - [ ] 网络异常catch并返回NetworkError
  - [ ] 统一错误响应格式（MSG_ERROR + ErrorBody）
- **验收标准**: 任何异常不会导致进程崩溃

### Task 5.6: 日志埋点完善
- **优先级**: P1
- **预估**: 2小时
- **依赖**: Task 5.4
- **交付物**:
  - [ ] 所有关键路径都有日志输出
  - [ ] 连接建立/断开日志
  - [ ] 登录成功/失败日志（不含密码）
  - [ ] 弹幕收发日志
  - [ ] 房间操作日志
  - [ ] 错误日志包含足够上下文信息
- **验收标准**: 出问题时能通过日志定位原因

### Task 5.7: 配置项外部化检查
- **优先级**: P1
- **预估**: 2小时
- **依赖**: Task 5.3
- **交付物**:
  - [ ] 扫描代码确认无硬编码IP/端口/密码
  - [ ] 所有可配置项都能从config/app.json读取
  - [ ] 敏感配置支持环境变量覆盖
- **验收标准**: 符合00_开发规范.md第8节约束

---

## 💬 Phase 6: 业务层-弹幕模块 (P0 - 3天)

### Task 6.1: 弹幕输入校验
- **优先级**: P0
- **预估**: 2小时
- **依赖**: Phase 5完成
- **交付物**:
  - [ ] validateInput(content): 非空检查、长度≤512字符
  - [ ] 返回Result<void>，失败时携带明确错误信息
- **测试用例**: 空内容/超长内容/正常内容

### Task 6.2: 限流功能实现
- **优先级**: P0
- **预估**: 4小时
- **依赖**: Task 2.8
- **交付物**:
  - [ ] checkRateLimit(userId, roomId):
    - 先检查用户级限流：1秒最多1条
    - 再检查房间级限流：单房间每秒1000条
    - 使用Redis INCR + EXPIRE 实现滑动窗口
  - [ ] 触发限流返回RateLimited错误码
  - [ ] 限流参数可配置
- **测试用例**:
  - 正常频率弹幕通过
  - 高频刷屏被拦截
  - 限流窗口过期后恢复
- **参考文档**: 06_弹幕模块设计.md 第4节

### Task 6.3: 弹幕过滤与广播
- **优先级**: P0
- **预估**: 4小时
- **依赖**: Task 6.1, 6.2, Phase 8(敏感词过滤)
- **交付物**:
  - [ ] processDanmaku(roomId, userId, userName, content)完整流程：
    1. 输入校验
    2. 限流检查
    3. 敏感词过滤（调用FilterService）
    4. 构造DanmakuBody Protobuf消息
    5. 调用RoomService.getOnlineUsers(roomId)获取在线fd列表
    6. 调用ConnectionManager.broadcast(roomId, message)广播
    7. 异步写Redis最近弹幕缓存
    8. 异步写MySQL danmaku_messages表
  - [ ] 广播同步执行（保证实时性）
  - [ ] 存储异步执行（不阻塞主链路）
- **参考文档**: 06_弹幕模块设计.md 第3节

### Task 6.4: 弹幕存储策略
- **优先级**: P0
- **预估**: 3小时
- **依赖**: Task 2.7, 2.8
- **交付物**:
  - [ ] saveToCache(): LPUSH到room:recent_danmaku:{roomId}，保留最近50条，TTL=5分钟
  - [ ] saveToDatabase(): INSERT INTO danmaku_messages表
  - [ ] 存储失败时记录补偿任务（不阻塞广播）
  - [ ] 广播成功但存储失败的容错处理
- **参考文档**: 06_弹幕模块设计.md 第5节

### Task 6.5: 最近弹幕查询接口
- **优先级**: P1
- **预估**: 2小时
- **依赖**: Task 6.4
- **交付物**:
  - [ ] queryRecentDanmaku(roomId, count): 优先从Redis查，miss则查MySQL
  - [ ] 用于客户端进入房间时快速回放历史弹幕
- **验收标准**: 进入房间能看到最近N条弹幕

### Task 6.6: 弹幕模块集成测试
- **优先级**: P0
- **预估**: 3小时
- **依赖**: Task 6.3
- **交付物**:
  - [ ] 端到端测试：客户端发弹幕 → 服务端处理 → 广播给其他客户端
  - [ ] 限流测试：高频刷屏拦截
  - [ ] 敏感词测试：命中替换/拦截
  - [ ] 存储验证：弹幕确实写入MySQL和Redis
  - [ ] 性能测试：万级QPS下延迟<50ms
- **里程碑**: **M2: 弹幕主链路跑通**

---

## 🏠 Phase 7: 业务层-房间模块 (P0 - 3天)

### Task 7.1: 房间状态机实现
- **优先级**: P0
- **预估**: 4小时
- **依赖**: Phase 5完成
- **交付物**:
  - [ ] RoomState枚举：CREATED(0) → READY(1) → LIVE(2) → OFFLINE(3) → CLOSED(4)
  - [ ] validateStateTransition(roomId, from, to)状态转移校验
  - [ ] 只允许顺序转移，CLOSED不可逆
  - [ ] 状态变更时更新Redis缓存和MySQL
- **测试用例**:
  - 所有合法转移路径
  - 非法转移拒绝（如CREATED直接到OFFLINE）
  - CLOSED后再操作拒绝
- **参考文档**: 07_房间模块设计.md 第2节

### Task 7.2: 房间CRUD操作
- **优先级**: P0
- **预估**: 4小时
- **依赖**: Task 7.1, 2.5
- **交付物**:
  - [ ] createRoom(hostUserId, roomName): 生成roomId → 写入DB → 初始化Redis缓存
  - [ ] startLive(roomId): CREATED/READY → LIVE
  - [ ] stopLive(roomId): LIVE → OFFLINE
  - [ ] closeRoom(roomId): 任何状态 → CLOSED（不可逆）
  - [ ] getRoomInfo(roomId): 查询房间详细信息
- **权限控制**: 只有主播才能操作自己的房间

### Task 7.3: 成员管理（进房/退房）
- **优先级**: P0
- **预估**: 4小时
- **依赖**: Task 7.2, 2.6, 2.8
- **交付物**:
  - [ ] joinRoom(roomId, userId):
    1. 校验房间状态（不能是CLOSED）
    2. INSERT ON DUPLICATE KEY UPDATE写入room_members表
    3. Redis SADD room:members:{roomId}（当日成员集合）
    4. Redis SADD room:online:{roomId}（在线集合）
    5. HINCRBY room:info:{roomId} online_count 1
    6. 广播给房间内其他用户"XXX进入房间"
  - [ ] leaveRoom(roomId, userId):
    1. 更新room_members.leave_time
    2. SREM room:online:{roomId}
    3. HINCRBY room:info:{roomId} online_count -1
    4. 广播"XXX离开房间"
- **参考文档**: 07_房间模块设计.md 第3-4节

### Task 7.4: 房间列表查询
- **优先级**: P0
- **预估**: 3小时
- **依赖**: Task 7.2
- **交付物**:
  - [ ] queryRoomList(page, pageSize, stateFilter, hostId, keyword, sortBy):
    - 支持按状态筛选（LIVE/OFFLINE等）
    - 支持按主播ID筛选
    - 支持按房间名关键词搜索
    - 支持按在线人数/开播时间排序
    - 返回总数、当前页数据、总页数
  - [ ] 热门房间列表可走Redis缓存加速
- **测试用例**: 各种筛选组合查询

### Task 7.5: 在线人数多层级同步
- **优先级**: P0
- **预估**: 4小时
- **依赖**: Task 7.3, 4.9
- **交付物**:
  - [ ] 本地Connection映射：实时精确在线数（权威数据源）
  - [ ] Redis room:info:{roomId}.online_count：1~3秒延迟近似值
  - [ ] MySQL rooms.online_count：10~30秒批量更新的持久化值
  - [ ] 定时线程每10-30秒扫描变化房间批量更新MySQL
  - [ ] 房间关闭时强制同步一次MySQL
- **更新流程**（用户进房）:
  ```
  1. 更新本地Connection映射（即时）
  2. Redis SADD + HINCRBY（即时）
  3. MySQL异步批量更新（延迟10-30秒）
  ```
- **参考文档**: 07_房间模块设计.md 第8节

### Task 7.6: 房间模块集成测试
- **优先级**: P0
- **预估**: 3小时
- **依赖**: Task 7.5
- **交付物**:
  - [ ] 创建房间 → 开播 → 进房 → 发弹幕 → 下播 → 关闭 全流程
  - [ ] 多用户同时进房退房
  - [ ] 房间关闭后拒绝新用户进入
  - [ ] 在线人数准确性验证
  - [ ] 异常断线后的在线人数修正
- **里程碑**: **M3: 房间主链路跑通**

---

## 🔒 Phase 8: 业务层-敏感词过滤 (P1 - 3天)

### Task 8.1: Trie树数据结构
- **优先级**: P1
- **预估**: 4小时
- **依赖**: 无
- **交付物**:
  - [ ] TrieNode节点定义：children[128], isEnd, wordId, level
  - [ ] buildTrie(words): 按字符逐层插入Trie，标记终止节点
  - [ ] 词库从文件加载（每行一个词，格式：word,level）
  - [ ] level含义：1=低级(替换), 2=中级(替换), 3=高级(拦截)
- **参考文档**: 08_敏感词过滤设计.md 第2节, 04_核心数据结构设计.md 第4节

### Task 8.2: AC自动机构建
- **优先级**: P1
- **预估**: 4小时
- **依赖**: Task 8.1
- **交付物**:
  - [ ] buildFailPointers(root): BFS构建fail指针
  - [ ] acMatch(root, text): O(n)时间复杂度的一次扫描多模式匹配
  - [ ] 返回FilterResult：filteredText, wasBlocked, hitPositions, maxLevel
  - [ ] 替换策略：低级词用*号替换，高级词整条拦截
- **测试用例**:
  - 单敏感词命中
  - 多敏感词同时命中
  - 敏感词嵌套/重叠
  - 无敏感词的正常文本
  - 超长文本（10000字符）性能测试
- **参考文档**: 08_敏感词过滤设计.md 第3节

### Task 8.3: 词库加载与热更新
- **优先级**: P1
- **预估**: 3小时
- **依赖**: Task 8.2
- **交付物**:
  - [ ] initialize(dictPath): 启动时加载词库，构建AC自动机
  - [ ] reloadDictionary(newPath): 后台构建新树，原子指针替换旧树
  - [ ] 使用atomic<AcNode*>实现无锁读取
  - [ ] 旧版本在无读者后自动释放（shared_ptr引用计数）
  - [ ] 词库版本号递增，便于审计
- **热更新约束**:
  - 不阻塞主业务线程
  - 必须可回滚
  - 版本号可记录
- **参考文档**: 08_敏感词过滤设计.md 第4节

### Task 8.4: FilterService完整封装
- **优先级**: P1
- **预估**: 2小时
- **依赖**: Task 8.3
- **交付物**:
  - [ ] FilterService类对外接口：
    - initialize(dictPath)
    - filterText(roomId, text) → Result<FilterResult>
    - reloadDictionary(newPath)
    - getDictionaryVersion()
  - [ ] 线程安全保证
- **验收标准**: Service层可直接调用filterText()

### Task 8.5: 敏感词过滤单元测试
- **优先级**: P1
- **预估**: 3小时
- **依赖**: Task 8.4
- **交付物**:
  - [ ] Trie构建正确性测试
  - [ ] AC自动机匹配完整性测试（≥30个用例）
  - [ ] 中文/英文/混合文本测试
  - [ ] 热更新无崩溃测试
  - [ ] 并发调用线程安全测试
  - [ ] 性能测试：单条过滤耗时<1ms
- **覆盖率目标**: ≥95%（核心算法）

---

## ⚡ Phase 9: 性能优化 (P1 - 4天)

### Task 9.1: 性能基准测试脚本开发
- **优先级**: P1
- **预估**: 4小时
- **依赖**: Phase 6, 7基本完成
- **交付物**:
  - [ ] 自研TCP压测客户端（模拟大量连接和弹幕发送）
  - [ ] 支持配置：并发连接数、弹幕频率、测试时长
  - [ ] 输出报告：QPS、延迟分布(P99/P95/P50)、错误率、内存/CPU占用
- **参考文档**: 10_性能优化方案.md 第3节

### Task 9.2: V1.0-V2.0稳定性优化
- **优先级**: P1
- **预估**: 6小时
- **依赖**: Task 9.1
- **交付物**:
  - [ ] 连接管理优化：减少锁竞争，优化Connection生命周期
  - [ ] 定时器优化：确保心跳检测准时
  - [ ] 日志优化：异步日志写入，避免I/O阻塞主线程
  - [ ] 错误处理完善：所有边界case都有fallback
- **压测指标**: 稳定运行24小时无崩溃

### Task 9.3: V3.0吞吐量优化
- **优先级**: P1
- **预估**: 8小时
- **依赖**: Task 9.2压测结果
- **交付物**（按需选择，必须有压测数据支撑）:
  - [ ] 广播路径减少拷贝（move语义、零拷贝）
  - [ ] 任务队列升级为无锁队列（如mpsc_queue）
  - [ ] 热点容器优化（如ConnectionManager改用更高效的数据结构）
  - [ ] 批量写入数据库（攒批insert）
  - [ ] Protobuf对象池复用（避免频繁分配释放）
- **优化原则**: 先定位瓶颈再优化，每次优化有前后对比数据

### Task 9.4: 内存与资源优化
- **优先级**: P1
- **预估**: 6小时
- **依赖**: Task 9.3
- **交付物**:
  - [ ] 内存池/对象池实现（Connection/Task/Message等高频对象）
  - [ ] 读缓冲区复用（避免每个连接频繁malloc）
  - [ ] 降低锁粒度（细粒度锁替代全局大锁）
  - [ ] 减少频繁的动态分配释放
- **目标**: 单连接内存占用<20KB

### Task 9.5: 监控指标采集
- **优先级**: P2
- **预估**: 4小时
- **依赖**: 无
- **交付物**:
  - [ ] PerformanceMonitor类
  - [ ] 采集指标：在线连接数/QPS/延迟/错误率/CPU/内存
  - [ ] 定期输出到日志或Prometheus（可选）
  - [ ] 支持运行时查看当前状态（如admin命令或HTTP接口）
- **参考文档**: 12_后期扩展方案.md 第4节

### Task 9.6: 最终压测验证
- **优先级**: P0
- **预估**: 4小时
- **依赖**: Task 9.3, 9.4
- **交付物**:
  - [ ] 完整压测报告
  - [ ] 达标项：
    - ✅ 单机长连接：5万+
    - ✅ 单条弹幕延迟：<50ms (P99)
    - ✅ 单机弹幕吞吐：10万条/秒
    - ✅ 敏感词过滤：<1ms/条
    - ✅ 内存占用：<2GB/万连接
    - ✅ 稳定运行24小时
- **里程碑**: **M4: 性能达标**

---

## 🧪 Phase 10: 测试与质量保障 (P0 - 5天，贯穿全程)

### Task 10.1: 单元测试框架搭建
- **优先级**: P0
- **预估**: 2小时
- **依赖**: Task 0.1
- **交付物**:
  - [ ] Google Test集成到CMake
  - [ ] tests/目录结构
  - [ ] 基础测试Main函数
  - [ ] CI脚本：`ctest --output-on-failure`
- **验收标准**: `make test`能运行测试

### Task 10.2: 核心模块单元测试
- **优先级**: P0
- **预估**: 8小时（持续增加）
- **依赖**: 各Phase完成时立即编写
- **交付物**（必须覆盖）:
  - [ ] 协议编解码（MessageCodec）≥15个用例
  - [ ] Trie构建与AC自动机匹配≥30个用例
  - [ ] 登录鉴权与连接状态切换≥10个用例
  - [ ] Header与Body字段一致性≥5个用例
  - [ ] 线程池任务调度≥8个用例
  - [ ] 房间状态机≥10个用例
  - [ ] Result和错误码转换≥10个用例
  - [ ] 限流算法≥5个用例
- **覆盖率目标**: 核心代码≥80%, 工具函数≥90%
- **参考文档**: 11_测试方案.md 第2节

### Task 10.3: 集成测试用例开发
- **优先级**: P0
- **预估**: 6小时
- **依赖**: Phase 5, 6, 7完成
- **交付物**:
  - [ ] 核心场景自动化测试脚本：
    1. 客户端连接服务器 ✓
    2. 登录成功并切换AUTHED状态 ✓
    3. 进入房间 ✓
    4. 发送弹幕 ✓
    5. 敏感词过滤命中与替换 ✓
    6. 房间广播成功 ✓
    7. 数据库落库成功 ✓
    8. 心跳超时后断开 ✓
  - [ ] 异常场景测试：
    - 粘包拆包异常
    - 非法消息类型
    - 未登录连接发业务消息
    - Header与Body不一致
    - 房间关闭后发弹幕
    - Redis/MySQL连接失败
    - 单用户高频刷屏
- **参考文档**: 11_测试方案.md 第3节

### Task 10.4: 压测脚本开发与执行
- **优先级**: P0
- **预估**: 6小时
- **依赖**: Task 9.1
- **交付物**:
  - [ ] 连接数压测：逐步增加到5万并发
  - [ ] 弹幕QPS压测：逐步增加到10万/秒
  - [ ] 广播延迟压测：万级在线时广播延迟
  - [ ] 过滤吞吐压测：AC自动机匹配速度
  - [ ] 长时间稳定压测：24小时连续运行
  - [ ] 每次压测记录：版本号/配置/结果/对比
- **参考文档**: 10_性能优化方案.md 第3节

### Task 10.5: 内存泄漏检测
- **优先级**: P0
- **预估**: 3小时
- **依赖**: Phase 6, 7完成
- **交付物**:
  - [ ] Valgrind memcheck检测（Linux）
  - [ ] AddressSanitizer检测（编译选项 -fsanitize=address）
  - [ ] 长时间运行监控内存增长趋势
  - [ ] 修复所有发现的内存泄漏
- **验收标准**: Valgrind 0 leak, 长时间运行内存平稳

### Task 10.6: 竞态条件检测
- **优先级**: P0
- **预估**: 3小时
- **依赖**: Phase 4, 5完成
- **交付物**:
  - [ ] ThreadSanitizer检测（编译选项 -fsanitize=thread）
  - [ ] 高并发场景专项测试（1000连接同时操作）
  - [ ] 修复所有数据竞争问题
- **验收标准**: TSan 0 warning

### Task 10.7: 安全漏洞扫描
- **优先级**: P1
- **预估**: 2小时
- **依赖**: Phase 4完成
- **交付物**:
  - [ ] SQL注入测试（所有数据库操作）
  - [ ] 缓冲区溢出测试（消息大小限制）
  - [ ] 权限绕过测试（未认证连接尝试操作）
  - [ ] 输入 fuzzing 测试（畸形包）
- **验收标准**: 无已知安全漏洞

### Task 10.8: 代码审查与文档更新
- **优先级**: P0
- **预估**: 4小时
- **依赖**: 全部开发完成
- **交付物**:
  - [ ] 全部代码通过Code Review（参照BACKEND_RULES.md第12节Checklist）
  - [ ] 代码注释完善（复杂逻辑说明"为什么"）
  - [ ] API文档更新（如果有对外接口）
  - [ ] 设计文档更新（如有偏离原设计的实现）
- **里程碑**: **M5: 发布就绪**

---

## 📊 里程碑总结

| 里程碑 | 时间点 | 交付标准 | 对应阶段 |
|--------|--------|----------|----------|
| **M0: 基础就绪** | Week 1末 | CMake编译通过、日志/配置/Result就绪 | Phase 0-1 |
| **M1: 主链路跑通** | Week 2末 | 连接→登录→发消息→收响应全通 | Phase 2-5 |
| **M2: 弹幕就绪** | Week 3末 | 弹幕收发/过滤/广播/存储全流程OK | Phase 6 |
| **M3: 房间就绪** | Week 4末 | 房间CRUD/成员管理/在线人数同步OK | Phase 7 |
| **M4: 性能达标** | Week 5末 | 压测全部达标，优化完成 | Phase 8-9 |
| **M5: 发布就绪** | Week 6末 | 测试通过/Bug清零/文档完整 | Phase 10 |

---

## 🎯 开发优先级矩阵

### 必须完成（P0 - 阻塞发布）
- ✅ Phase 0-5: 基础设施 + 主链路
- ✅ Phase 6-7: 弹幕 + 房间核心业务
- ✅ Phase 10: 测试与质量保障

### 应该完成（P1 - 强烈推荐）
- ✅ Phase 8: 敏感词过滤（内容合规必需）
- ✅ Phase 9: 性能优化（用户体验必需）

### 可以延后（P2 - 后续迭代）
- ⏰ 礼物系统增强
- ⏰ 监控告警系统
- ⏰ 分布式扩展
- ⏰ AI模块接入

---

## ⚠️ 关键风险与应对

| 风险 | 影响 | 概率 | 应对措施 |
|------|------|------|----------|
| 协议对接不一致 | 联调困难 | 中 | 严格遵循02_网络协议设计.md，联调前先对齐Proto文件 |
| 并发Bug难以复现 | 稳定性问题 | 高 | 充分使用TSan/Valgrind，高并发专项测试 |
| 性能不达标 | 无法上线 | 中 | 尽早开始压测(V1.0后就压)，留足优化时间 |
| 第三方库兼容性问题 | 编译失败 | 低 | vcpkg锁定版本，Docker环境统一 |

---

## 📞 技术支持与文档索引

**开发时必读文档**:
1. **BACKEND_RULES.md** - 后端开发规则手册（本文档的上位规范）
2. **00_开发规范.md** - 代码风格最高约束
3. **01_项目架构设计.md** - 架构决策依据
4. **02_网络协议设计.md** - 协议实现的唯一标准
5. **03_数据库设计.md** - 数据库Schema定义
6. **DEVELOPER_RULES.md** - 通用工程师规则

**遇到问题时**:
- 编译问题 → 查看09_构建与部署.md
- 性能问题 → 查看10_性能优化方案.md
- 测试问题 → 查看11_测试方案.md
- 扩展问题 → 查看12_后期扩展方案.md

---

**🎉 祝后端开发顺利！记住：先跑通主链路，再追求完美。**
