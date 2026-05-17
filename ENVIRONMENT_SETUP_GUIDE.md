# C++ 直播项目开发环境配置指南

## 📋 当前环境状态总览

| 组件 | 状态 | 版本/路径 |
|------|------|-----------|
| **编译器** | ✅ 已安装 | MSVC 19.29.30159 (VS2019) |
| **CMake** | ✅ 已安装 | 3.20.21032501 |
| **Git** | ✅ 已安装 | 2.51.2.windows.1 |
| **vcpkg** | ⏳ 正在下载 | D:\vcpkg (克隆中) |
| **项目结构** | ✅ 已创建 | chatroom-server/ |

---

## 🔧 已完成的工作

### 1. 编译器环境
- **Visual Studio 2019** 已检测到（路径：`D:\vs2019\1IDE`）
- **MSVC 版本**: 14.29.30133（完全支持 C++17 标准）
- 虽然文档推荐 VS2022，但 VS2019 完全满足项目需求

### 2. 构建工具
- **CMake**: 使用 VS2019 自带版本（`D:\vs2019\1IDE\...\cmake.exe`）
- 版本：3.20（略低于文档建议的 3.26+，但功能完整可用）

### 3. 项目目录结构
已按照 [01_项目架构设计.md](01_项目架构设计.md) 规范创建：

```
chatroom-server/
├── CMakeLists.txt          ✅ 根构建文件
├── config/
│   └── app.json            ✅ 运行时配置
├── include/
│   ├── common/
│   │   ├── result.h        ✅ 错误处理封装
│   │   └── logger.h        ✅ 日志接口定义
│   ├── net/                ⬜ 网络层头文件
│   ├── protocol/           ⬜ 协议层头文件
│   ├── service/            ⬜ 业务层头文件
│   ├── data/               ⬜ 数据层头文件
│   └── util/               ⬜ 工具类头文件
├── src/
│   ├── main.cpp            ✅ 程序入口
│   ├── common/
│   │   ├── logger.cpp      ✅ 日志实现
│   │   └── result.cpp      ✅ Result 模板实例化
│   └── ...                 ⬜ 其他模块实现
├── proto/                  ⬜ Protobuf 定义
├── tests/                  ⬜ 单元测试
├── tools/                  ⬜ 辅助工具
└── scripts/
    └── build.bat           ✅ Windows 构建脚本
```

### 4. 核心代码框架
已创建符合 [00_开发规范.md](00_开发规范.md) 的基础代码：

- **Result<T> 模板类**：统一错误码封装
- **Logger 类**：基于 spdlog 的日志系统
- **main.cpp**：程序启动入口示例

---

## ⚠️ 待完成的步骤

### 步骤 1：完成 vcpkg 安装（预计需要 5-15 分钟）

vcpkg 仓库正在后台克隆到 `D:\vcpkg`。完成后需手动执行：

```bash
# 进入 vcpkg 目录
cd D:\vcpkg

# 执行引导脚本（Windows）
.\bootstrap-vcpkg.bat -disableMetrics

# 集成到系统
.\vcpkg integrate install
```

**或者直接运行我创建的脚本：**
```bash
d:\ai直播项目\setup_vcpkg.bat
```

### 步骤 2：安装第三方依赖库

vcpkg 就绪后，运行：

```bash
d:\ai直播项目\install_dependencies.bat
```

这将自动安装以下 8 个依赖库：

| 库名 | 用途 | 推荐版本 |
|------|------|----------|
| spdlog | 高性能日志 | 1.14+ |
| fmt | 格式化输出 | 11.x |
| protobuf | 序列化协议 | 27+ |
| gtest | 单元测试 | 1.14+ |
| yaml-cpp | YAML 配置解析 | 0.8+ |
| nlohmann-json | JSON 处理 | 3.11+ |
| hiredis | Redis 客户端 | 1.2+ |
| mysql-connector-cpp | MySQL 连接器 | 8.0+ |

### 步骤 3：数据库准备（可选）

如果本地有 MySQL 和 Redis，可跳过此步。否则需要：

**MySQL 8.0+：**
- 安装地址：https://dev.mysql.com/downloads/installer/
- 创建数据库：`chatroom_db`
- 创建用户：`chatroom@localhost`
- 导入表结构（参考 [03_数据库设计.md](03_数据库设计.md)）

**Redis 7.x：**
- 安装地址：https://redis.io/download/
- 默认端口：6379
- 无需密码（开发环境）

### 步骤 4：编译项目

所有依赖就绪后：

```bash
# 方式一：使用构建脚本（推荐）
cd d:\ai直播项目\chatroom-server
..\scripts\build.bat

# 方式二：手动构建
cd d:\ai直播项目\chatroom-server\build
cmake .. -G "Visual Studio 16 2019" -A x64 ^
    -DCMAKE_TOOLCHAIN_FILE=D:/vcpkg/scripts/buildsystems/vcpkg.cmake ^
    -DBUILD_TESTS=ON
cmake --build . --config Release --parallel 8
```

---

## 🛠️ 辅助脚本说明

我已在项目根目录创建了以下自动化脚本：

1. **setup_environment.bat**  
   一键安装 VS Build Tools、CMake、Git、Python  
   （⚠️ 当前网络问题可能失败，建议手动安装）

2. **setup_vcpkg.bat**  
   克隆并初始化 vcpkg 包管理器

3. **install_dependencies.bat**  
   自动安装全部 8 个第三方依赖库

4. **verify_environment.bat**  
   验证所有工具是否正确安装

5. **chatroom-server/scripts/build.bat**  
   项目专用构建脚本（自动配置 MSVC + vcpkg）

---

## 📝 开发规范要点提醒

根据你的 12 个文档，请务必遵守：

✅ **C++17 标准** - 所有新代码必须使用 C++17 特性  
✅ **命名规范**：
   - 类名：PascalCase（如 `RoomManager`）
   - 函数：camelCase 动词开头（如 `handleConnect()`）
   - 成员变量：`m_` 前缀（如 `m_roomId`）
   - 常量：全大写（如 `MAX_ROOM_COUNT`）

✅ **错误处理**：统一使用 `Result<T>` + `ErrorCode`，禁止异常流控制  
✅ **智能指针**：优先 `unique_ptr`，必要时 `shared_ptr`  
✅ **日志格式**：`[时间][级别][模块][线程ID][消息]`  
✅ **命名空间**：所有业务代码放在 `chatroom` 命名空间下  

---

## 🎯 下一步行动建议

1. **立即执行**：等待 vcpkg 下载完成后运行 `setup_vcpkg.bat`
2. **然后执行**：`install_dependencies.bat` 安装所有依赖
3. **最后验证**：运行 `verify_environment.bat` 确认一切就绪
4. **开始编码**：按照文档顺序从 [02_网络协议设计.md](02_网络协议设计.md) 开始实现

---

## ❓ 常见问题

**Q: 为什么用 VS2019 而不是 VS2022？**  
A: 你的系统已安装 VS2019 且完全支持 C++17，无需额外下载 6GB+ 的 VS2022。

**Q: CMake 3.20 是否够用？**  
A: 够用。虽然文档推荐 3.26+，但 3.20 支持所有必要的 CMake 功能。

**Q: 如果网络不稳定导致 vcpkg 下载失败？**  
A: 可以使用国内镜像源加速：
```bash
git clone https://gitee.com/mirrors/vcpkg.git D:\vcpkg
```

**Q: 如何验证环境完全就绪？**  
A: 运行 `verify_environment.bat`，看到所有项都显示 ✅ 即可。

---

## 📞 技术支持

如遇到问题，可检查：
1. 日志文件：`chatroom-server/logs/chatroom.log`
2. CMake 缓存：`chatroom-server/build/CMakeCache.txt`
3. vcpkg 日志：`D:\vcpkg\buildtrees\`

祝开发顺利！🚀
