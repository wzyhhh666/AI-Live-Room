# 直播弹幕系统 — 软件开发工程师开发规则手册

> **版本**: v1.0
> **最后更新**: 2026-05-17
> **适用角色**: 全栈软件工程师、后端工程师、前端工程师、测试工程师
> **文档优先级**: 最高（所有开发活动必须遵循本手册）

---

## 📋 目录

1. [项目概述](#1-项目概述)
2. [技术栈与架构](#2-技术栈与架构)
3. [开发环境配置](#3-开发环境配置)
4. [代码规范总纲](#4-代码规范总纲)
5. [开发工作流程](#5-开发工作流程)
6. [文档体系与阅读顺序](#6-文档体系与阅读顺序)
7. [Git版本控制规范](#7-git版本控制规范)
8. [代码审查标准](#8-代码审查标准)
9. [测试规范](#9-测试规范)
10. [部署与发布流程](#10-部署与发布流程)
11. [性能目标与优化路线](#11-性能目标与优化路线)
12. [安全规范](#12-安全规范)
13. [常见问题与解决方案](#13-常见问题与解决方案)

---

## 1. 项目概述

### 1.1 项目定位
**C++17 高性能直播弹幕+房间系统**，采用 Reactor + 线程池 + 消息队列的经典高并发架构。

### 1.2 核心功能模块
| 模块 | 功能描述 | 优先级 |
|------|----------|--------|
| 用户登录鉴权 | SHA-256密码哈希 + Token会话管理 | P0 |
| 房间管理 | 创建/开播/下播/关闭状态机 | P0 |
| 弹幕系统 | 发送/过滤(AC自动机)/广播/存储 | P0 |
| 礼物系统 | 送礼/排行榜/积分 | P1 |
| 敏感词过滤 | Trie树 + AC自动机实时过滤 | P0 |
| 在线人数管理 | 实时统计/多层级同步 | P0 |
| 心跳保活 | TCP长连接维护 | P0 |

### 1.3 系统架构图
```
┌─────────────────────────────────────────┐
│         客户端 (TCP 长连接)              │
└────────────────┬────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────┐
│      网络层 / Reactor 主线程             │
│   epoll监听 / 收包 / 心跳 / 广播分发     │
└────────────────┬────────────────────────┘
                 │ 解析后消息
                 ▼
┌─────────────────────────────────────────┐
│        消息队列 / 任务分发层             │
│  连接事件 / 弹幕事件 / 房间事件 / 定时器  │
└────────────────┬────────────────────────┘
                 │ 任务对象
                 ▼
┌─────────────────────────────────────────┐
│       工作线程池 / 业务处理层            │
│ 弹幕过滤 / 房间更新 / 存储写入 / 排行榜  │
└────────────────┬────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────┐
│          数据层 / 缓存层                 │
│      MySQL 8.0 + Redis 7.x + 本地索引   │
└─────────────────────────────────────────┘
```

---

## 2. 技术栈与架构

### 2.1 后端技术栈
| 组件 | 版本 | 用途 |
|------|------|------|
| C++标准 | C++17 | 语言标准 |
| 编译器 | GCC 13+ / MSVC 2022+ | 编译器 |
| 构建工具 | CMake 3.26+ / Ninja | 构建系统 |
| 包管理 | vcpkg | 第三方库管理 |
| 网络模型 | epoll (Linux) / IOCP (Windows) | I/O多路复用 |
| 序列化 | Protobuf 27+ | 协议序列化 |
| 数据库 | MySQL 8.0 | 持久化存储 |
| 缓存 | Redis 7.x | 热数据缓存 |
| 日志 | spdlog 1.14+ | 日志框架 |

### 2.2 核心第三方库清单
```cmake
# vcpkg 依赖列表
spdlog[core,fmt,tz-offset]:x64-linux    # 日志库
fmt:x64-linux                            # 格式化库
nlohmann-json:x64-linux                  # JSON处理
gtest:x64-linux                          # 测试框架
yaml-cpp:x64-linux                       # YAML配置
protobuf                                # 协议序列化
hiredis                                 # Redis客户端
mysqlconnector                          # MySQL连接器
```

### 2.3 架构分层原则
```
网络层 → 协议层 → 业务层 → 数据层
```

**单向依赖原则**：
- ✅ 网络层可以调用协议层
- ❌ 数据层禁止反向依赖业务层
- ❌ 协议层禁止直接包含业务逻辑
- ❌ 网络层禁止直接访问MySQL或Redis

---

## 3. 开发环境配置

### 3.1 Docker开发环境（推荐）
项目已配置完整的Docker开发环境，包含：

**服务容器**：
- `chatroom-dev`: C++17开发环境容器
- `chatroom-mysql`: MySQL 8.0数据库
- `chatroom-redis`: Redis 7缓存

**快速启动命令**：
```bash
cd docker
docker compose up -d
docker exec -it chatroom-dev bash
```

**容器内已安装**：
- ✅ GCC 12.3.0 (C++17支持)
- ✅ G++ 12.3.0
- ✅ CMake 3.22.1
- ✅ Ninja Build
- ✅ vcpkg及所有依赖库

### 3.2 本地开发环境
**Windows环境**：
- Visual Studio 2022 (MSVC编译器)
- CMake 3.26+
- vcpkg包管理器
- MySQL 8.0 Installer
- Redis 7.x (Windows版或WSL2)

**Linux环境** (Ubuntu 22.04+)：
```bash
sudo apt update
sudo apt install build-essential gcc-12 g++-12 cmake ninja-build \
    git curl wget pkg-config libssl-dev zlib1g-dev \
    libmysqlclient-dev libhiredis-dev protobuf-compiler
```

### 3.3 IDE推荐配置
- **VSCode**: 安装C/C++扩展、CMake Tools扩展
- **CLion**: JetBrains官方C/C++ IDE（推荐）
- **Visual Studio 2022**: Windows原生开发

---

## 4. 代码规范总纲

### 4.1 命名规范
| 类型 | 规则 | 示例 |
|------|------|------|
| 类名/结构体/枚举 | PascalCase | `RoomManager`, `DanmakuMessage` |
| 函数名 | camelCase, 动词开头 | `handleConnect()`, `sendBroadcast()` |
| 局部变量 | camelCase | `clientFd`, `messageCount` |
| 成员变量 | `m_`前缀 + camelCase | `m_roomId`, `m_onlineCount` |
| 常量 | 全大写下划线 | `MAX_ROOM_COUNT`, `DEFAULT_HEARTBEAT_MS` |
| 命名空间 | 小写单词 | `net`, `protocol`, `service`, `data` |

### 4.2 文件组织规范
```
chatroom-server/
├── include/
│   ├── common/          # 公共定义(Result, ErrorCode, Logging)
│   ├── net/             # 网络层(Connection, EpollServer)
│   ├── protocol/        # 协议层(Message, MessageCodec)
│   ├── service/         # 业务层(RoomService, DanmakuService)
│   ├── data/            # 数据层(Repository, Cache)
│   └── util/            # 工具类
├── src/
│   ├── main.cpp         # 程序入口
│   ├── net/             # 网络层实现
│   ├── protocol/        # 协议层实现
│   ├── service/         # 业务层实现
│   ├── data/            # 数据层实现
│   └── util/            # 工具类实现
├── proto/               # Protobuf定义文件
├── config/              # 运行配置文件
├── tests/               # 测试代码
├── tools/               # 压测/运维工具
└── third_party/         # 第三方源码
```

**文件命名规则**：
- 头文件: `.h`
- 实现文件: `.cpp`
- 头文件与实现文件一一对应
- 使用小写下划线风格: `room_manager.h`, `room_manager.cpp`

### 4.3 智能指针使用规则
```cpp
// 默认优先使用 unique_ptr (单一所有权)
std::unique_ptr<Connection> conn = std::make_unique<Connection>();

// 只在多方共享生命周期时使用 shared_ptr
std::shared_ptr<Connection> sharedConn = std::make_shared<Connection>();

// 非所有权引用使用裸指针或引用
void processUser(User* user);  // 明确标注"非owning"
```

### 4.4 错误处理策略
**统一采用错误码 + Result封装**：
```cpp
enum class ErrorCode {
    Success = 0,
    InvalidArgument,
    ProtocolError,
    NetworkError,
    DatabaseError,
    NotFound,
    PermissionDenied,
    RateLimited,
    InternalError
};

template <typename T>
class Result {
public:
    static Result ok(T value);
    static Result fail(ErrorCode code, std::string message);
    bool isOk() const;
    const T& value() const;
    ErrorCode code() const;
    const std::string& message() const;
};
```

**异常使用边界**：
- ✅ 业务层返回Result或错误码
- ✅ 启动期不可恢复错误可抛异常
- ❌ 禁止在消息循环中用异常做分支控制

### 4.5 日志规范
**日志级别**：
| 级别 | 用途 | 场景 |
|------|------|------|
| TRACE | 高频调试 | 默认关闭 |
| DEBUG | 开发调试 | 调试信息 |
| INFO | 正常关键事件 | 连接建立、登录成功 |
| WARN | 可恢复异常 | 限流触发、重试操作 |
| ERROR | 处理失败 | 数据库写入失败 |
| FATAL | 不可恢复错误 | 核心资源初始化失败 |

**日志格式**：
```
[时间][级别][模块][线程ID][fd/roomId][消息]
示例: [2026-05-15 10:31:22.143][INFO][net][tid=12][fd=1289] client connected
```

---

## 5. 开发工作流程

### 5.1 功能开发标准流程
```
1. 需求分析 → 阅读相关设计文档
2. 接口设计 → 定义头文件契约
3. 单元测试 → 先写测试用例
4. 代码实现 → 按规范编写实现
5. 自测验证 → 本地运行测试
6. 代码审查 → 提交MR/PR
7. 合并集成 → 通过CI检查
8. 联调测试 → 与前端联调
```

### 5.2 Bug修复流程
```
1. 问题复现 → 记录复现步骤
2. 定位根因 → 分析日志和代码
3. 编写测试 → 先写能暴露问题的测试
4. 修复代码 → 最小改动原则
5. 验证修复 → 运行全部测试
6. 代码审查 → 确认修复正确性
7. 发布修复 → 热修复或常规发布
```

### 5.3 代码提交频率
- **功能开发**: 每完成一个独立功能点就提交
- **Bug修复**: 修复一个问题就提交
- **重构**: 完成一个重构单元就提交
- **禁止**: 积累大量未提交代码

---

## 6. 文档体系与阅读顺序

### 6.1 核心文档列表（14个）
| 序号 | 文档名称 | 内容概要 | 阅读优先级 |
|------|----------|----------|------------|
| 00 | 开发规范.md | 命名、错误处理、日志、代码风格 | ⭐⭐⭐⭐⭐ 必读 |
| 01 | 项目架构设计.md | 分层架构、线程模型、目录结构 | ⭐⭐⭐⭐⭐ 必读 |
| 02 | 网络协议设计.md | TCP协议、消息格式、Protobuf定义 | ⭐⭐⭐⭐⭐ 必读 |
| 03 | 数据库设计.md | MySQL表结构、Redis数据结构 | ⭐⭐⭐⭐ 必读 |
| 04 | 核心数据结构设计.md | 线程池、Trie树、定时器、连接管理 | ⭐⭐⭐⭐ 推荐 |
| 05 | 接口统一标准.md | 函数命名、Result封装、回调签名 | ⭐⭐⭐⭐ 推荐 |
| 06 | 弹幕模块设计.md | 弹幕收发、广播策略、限流存储 | ⭐⭐⭐ 按需 |
| 07 | 房间模块设计.md | 状态机、成员管理、权限控制 | ⭐⭐⭐ 按需 |
| 08 | 敏感词过滤设计.md | Trie树、AC自动机、热更新 | ⭐⭐⭐ 按需 |
| 09 | 构建与部署.md | CMake、依赖管理、部署步骤 | ⭐⭐⭐ 推荐 |
| 10 | 性能优化方案.md | 性能指标、压测方案、优化路线 | ⭐⭐ 进阶 |
| 11 | 测试方案.md | 单元测试、集成测试、压测脚本 | ⭐⭐⭐ 推荐 |
| 12 | 后期扩展方案.md | 分布式、AI接入、监控告警 | ⭐ 进阶 |
| 13 | 客户端体系开发文档.md | 客户端协议对接、UI交互 | ⭐⭐⭐ 前端必读 |

### 6.2 推荐阅读顺序
```
第一轮（基础必读）:
00_开发规范 → 01_项目架构设计 → 02_网络协议设计 → 03_数据库设计

第二轮（接口规范）:
04_核心数据结构设计 → 05_接口统一标准

第三轮（业务模块）:
06_弹幕模块设计 → 07_房间模块设计 → 08_敏感词过滤设计

第四轮（工程化）:
09_构建与部署 → 11_测试方案

第五轮（进阶扩展）:
10_性能优化方案 → 12_后期扩展方案 → 13_客户端体系开发文档
```

**执行原则**: 先规范、后实现；先主链路、后扩展；先数据和协议、后业务细节。

---

## 7. Git版本控制规范

### 7.1 分支策略
```
main (生产分支)
  ├─ develop (开发分支)
  │    ├─ feature/xxx (功能分支)
  │    ├─ fix/xxx (修复分支)
  │    └─ refactor/xxx (重构分支)
  │
  └─ release/v1.0 (发布分支)
       └─ hotfix/xxx (紧急修复)
```

### 7.2 Commit Message规范
**格式**: `<type>(<scope>): <subject>`

**Type类型**:
| Type | 描述 | 示例 |
|------|------|------|
| feat | 新功能 | feat(login): 添加Token刷新机制 |
| fix | Bug修复 | fix(danmaku): 修复广播空指针崩溃 |
| docs | 文档变更 | docs(api): 更新协议文档 |
| style | 代码格式调整 | style(net): 统一命名风格 |
| refactor | 重构 | refactor(pool): 重构任务队列实现 |
| test | 测试相关 | test(filter): 添加AC自动机测试用例 |
| chore | 构建/工具 | chore(cmake): 更新CMake配置 |

**示例**:
```bash
git commit -m "feat(room): 添加房间状态机实现"
git commit -m "fix(protocol): 修复粘包处理边界条件"
git commit -m "test(danmaku): 添加限流功能单元测试"
```

### 7.3 Code Review checklist
- [ ] 代码符合命名规范
- [ ] 错误处理完整（Result封装）
- [ ] 日志输出符合格式要求
- [ ] 无硬编码配置项
- [ ] 边界条件已覆盖
- [ ] 内存无泄漏风险
- [ ] 线程安全性有保障
- [ ] 性能无明显退化
- [ ] 相关文档已更新

---

## 8. 代码审查标准

### 8.1 审查维度
| 维度 | 权重 | 说明 |
|------|------|------|
| 正确性 | 40% | 逻辑正确、边界处理完善 |
| 可读性 | 25% | 命名清晰、注释适当、结构合理 |
| 性能 | 20% | 无明显性能问题、算法选择合理 |
| 规范性 | 10% | 符合项目编码规范 |
| 可测试性 | 5% | 易于编写单元测试 |

### 8.2 常见问题清单
❌ **必须拒绝的问题**:
- 内存泄漏（智能指针使用不当）
- 竞态条件（缺少锁保护）
- 未处理的错误码
- 硬编码的密码/IP/端口
- SQL注入风险
- 缓冲区溢出风险

⚠️ **需要讨论的问题**:
- 过度设计的抽象
- 不必要的性能优化
- 复杂度过高的函数
- 缺少必要注释的逻辑

✅ **鼓励的做法**:
- 清晰的接口设计
- 完善的错误处理
- 适当的日志输出
- 简洁的实现方式
- 有意义的测试用例

---

## 9. 测试规范

### 9.1 测试金字塔
```
        /\
       /  \     E2E测试 (5%)
      /────\    集成测试 (15%)
     /      \   单元测试 (80%)
    /────────\
```

### 9.2 单元测试要求
**测试框架**: Google Test (gtest)

**必须测试的核心模块**:
- ✅ 协议编码与解码
- ✅ Trie构建与AC自动机匹配
- ✅ 登录鉴权与连接状态切换
- ✅ Header与Body字段一致性
- ✅ 线程池任务调度
- ✅ 房间状态机转移
- ✅ Result和错误码转换

**测试覆盖率目标**:
- 核心业务代码: ≥80%
- 工具函数: ≥90%
- 整体代码: ≥70%

### 9.3 集成测试场景
**核心场景（必须通过）**:
1. 客户端连接服务器
2. 登录成功并完成连接状态切换
3. 进入房间
4. 发送弹幕
5. 敏感词过滤命中与替换
6. 房间广播成功
7. 数据库落库成功
8. 心跳超时后断开

**异常场景（必须覆盖）**:
- 粘包拆包异常
- 非法消息类型
- 未登录连接发送业务消息
- Header与Body字段不一致
- 房间关闭后继续发弹幕
- Redis/MySQL连接失败
- 单用户高频刷屏

### 9.4 性能压测指标
| 指标 | 目标值 | 测试方法 |
|------|--------|----------|
| 单机长连接数 | 5万+ | 并发连接压测 |
| 单条弹幕延迟 | <50ms | 端到端耗时统计 |
| 单机弹幕吞吐 | 10万条/秒 | 高频发送压测 |
| 敏感词过滤耗时 | <1ms/条 | 单次过滤耗时 |
| 内存占用 | <2GB/万连接 | 内存监控 |

---

## 10. 部署与发布流程

### 10.1 构建步骤
```bash
# 1. 克隆代码
git clone <repo-url>
cd chatroom-server

# 2. 配置CMake
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake

# 3. 编译
cmake --build . --config Release -j$(nproc)

# 4. 运行测试
ctest --output-on-failure

# 5. 准备部署
cp ../config/* ./config/
```

### 10.2 Docker部署（生产环境）
```bash
# 构建镜像
docker build -t chatroom-server:v1.0 -f docker/Dockerfile .

# 启动服务
cd docker
docker compose up -d

# 验证服务
docker ps | grep chatroom
docker logs chatroom-dev
```

### 10.3 配置文件管理
**禁止硬编码**, 所有配置从文件读取：

```jsonc
{
  "server": { "host": "0.0.0.0", "port": 8900 },
  "mysql": { "host": "127.0.0.1", "port": 3306 },
  "redis": { "host": "127.0.0.1", "port": 6379 },
  "heartbeat": { "client_interval_sec": 10 },
  "log": { "level": "INFO" }
}
```

---

## 11. 性能目标与优化路线

### 11.1 分阶段优化目标
| 阶段 | 目标 | 关键指标 |
|------|------|----------|
| V1.0 | 基础可用 | 主链路跑通 |
| V2.0 | 稳定性 | 连接管理优化、错误处理完善 |
| V3.0 | 吞吐优化 | 广播路径减少拷贝、批量写入DB |
| V4.0 | 资源优化 | 内存池、缓冲区复用、降低锁粒度 |
| V5.0 | 架构优化 | 分片、多节点扩展 |

### 11.2 优化原则
- ✅ 先定位瓶颈，再决定优化方案
- ✅ 任何优化必须有压测前后对比数据
- ✅ 优先优化最热路径：接收→过滤→广播
- ❌ 不要没压测就改成复杂无锁结构
- ❌ 不要为了"性能高"提前做分布式

---

## 12. 安全规范

### 12.1 输入验证
- 所有外部输入必须校验长度和格式
- 消息大小设置上限（防恶意大包）
- SQL参数化查询（防SQL注入）
- 敏感词过滤必须在广播前执行

### 12.2 认证与授权
- Token有效期24小时，心跳不续期
- 未登录连接只能发MSG_LOGIN和MSG_HEARTBEAT
- 房间操作必须权限校验
- fd与userId绑定机制防止越权

### 12.3 数据保护
- 密码使用SHA-256哈希存储
- Token使用UUID或随机32字节hex
- 敏感配置不提交到代码仓库
- 日志不记录密码和Token明文

---

## 13. 常见问题与解决方案

### Q1: 如何快速搭建开发环境？
**A**: 使用Docker一键启动：
```bash
cd docker && docker compose up -d
docker exec -it chatroom-dev bash
```

### Q2: 编译时提示找不到第三方库？
**A**: 检查vcpkg是否正确配置：
```bash
# Linux
cmake .. -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake

# Windows
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

### Q3: 如何调试网络协议问题？
**A**: 
1. 开启TRACE级别日志查看原始数据包
2. 使用tcpdump抓包分析
3. 检查Header中的magic/version/bodyLen字段
4. 验证Protobuf序列化/反序列化结果

### Q4: 敏感词过滤性能如何保障？
**A**: 
1. 使用AC自动机实现O(n)时间复杂度
2. 词库加载用原子指针替换，支持无锁读取
3. 热更新时构建新树，完成后原子切换

### Q5: 如何处理大量连接同时断开？
**A**: 
1. 定时线程分批检测超时连接
2. 断线清理按正常离开流程执行
3. 批量清理避免瞬间Redis/MySQL压力过大
4. 设置合理的清理批次大小

### Q6: 房间在线人数不准确怎么办？
**A**: 
1. 以本地Connection映射为权威数据源
2. Redis和MySQL仅作为近似值
3. 定时线程每10-30秒批量同步MySQL
4. 房间关闭时强制同步一次

---

## 📚 附录

### A. 快速命令参考
```bash
# 开发环境
docker compose up -d                    # 启动所有服务
docker exec -it chatroom-dev bash       # 进入开发容器
docker compose down                     # 停止所有服务
docker compose logs -f                  # 查看日志

# 构建与测试
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure

# 代码质量
clang-format -i src/**/*.cpp            # 格式化代码
clang-tidy src/**/*.cpp                 # 静态分析
```

### B. 关键联系人
- **项目负责人**: [待填写]
- **后端负责人**: [待填写]
- **前端负责人**: [待填写]
- **运维负责人**: [待填写]

### C. 文档版本历史
| 版本 | 日期 | 作者 | 变更内容 |
|------|------|------|----------|
| v1.0 | 2026-05-17 | AI Assistant | 初始版本创建 |

---

**📌 重要提醒**: 本手册是项目最高优先级约束文档。所有开发活动必须严格遵循。如有疑问，请先查阅00~13号核心文档，或联系项目负责人。
