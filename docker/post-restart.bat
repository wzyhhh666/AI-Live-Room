@echo off
chcp 65001 >nul
title Docker 环境配置 - 重启后执行
color 0B

echo ╔══════════════════════════════════════════════════════╗
echo ║     重启后 - Docker 开发环境自动配置脚本             ║
echo ╚══════════════════════════════════════════════════════╝
echo.

:: 设置 PATH
set "PATH=%ProgramFiles%\Docker\Docker\resources\bin;%PATH%"

:: ====== 第 1 步: 检查 Docker ======
echo [1/5] 检查 Docker 状态...
docker --version >nul 2>&1
if %errorLevel% neq 0 (
    echo [错误] Docker 未找到! 请确认 Docker Desktop 已安装并启动
    pause
    exit /b 1
)
docker --version

:: ====== 第 2 步: 等待引擎就绪 ======
echo.
echo [2/5] 等待 Docker 引擎就绪...
set MAX_WAIT=120
for /l %%i in (1,1,%MAX_WAIT%) do (
    docker info >nul 2>&1
    if !errorLevel! equ 0 (
        <nul set /p "=等待中... %%i 秒" 
        timeout /t 1 /nobreak >nul
        echo.
    ) else (
        goto :engine_ready
    )
)
echo [超时] Docker 引擎未能在 %MAX_WAIT% 秒内启动
echo 请手动检查 Docker Desktop 是否正常运行
pause
exit /b 1

:engine_ready
echo ✅ Docker 引擎已就绪!
docker info | findstr "Server Version Containers"
echo.

:: ====== 第 3 步: 构建开发环境镜像 ======
echo ========================================
echo [3/5] 构建 C++17 开发环境镜像...
echo ========================================
echo 这可能需要 5-15 分钟 (取决于网速)
echo.

cd /d "%~dp0..\docker"

docker compose build chatroom-server
if %errorLevel% neq 0 (
    echo ❌ 镜像构建失败!
    pause
    exit /b 1
)
echo.
echo ✅ 镜像构建完成!
echo.

:: ====== 第 4 步: 启动所有服务 ======
echo ========================================
echo [4/5] 启动所有服务 (MySQL + Redis + 容器)...
echo ========================================
docker compose up -d
if %errorLevel% neq 0 (
    echo ❌ 服务启动失败!
    pause
    exit /b 1
)

echo.
echo 等待 MySQL 就绪...
timeout /t 15 /nobreak >nul
echo.
docker compose ps
echo.

:: ====== 第 5 步: 验证 ======
echo ========================================
echo [5/5] 验证环境...
echo ========================================
echo.
echo --- 容器内编译器版本 ---
docker compose exec chatroom-server gcc --version | head -1
echo.
echo --- 容器内 CMake 版本 ---
docker compose exec chatroom-server cmake --version | head -1
echo.
echo --- 已安装的依赖库 ---
docker compose exec chatroom-server bash -c "dpkg -l 2>/dev/null | grep -E 'spdlog|nlohmann|libfmt|protobuf|hiredis|mysql' | awk '{print \"  ✅ \"$2}' || true"
echo.
echo --- 数据库连接测试 ---
docker compose exec mysql mysql -uroot -proot123 -e "SELECT '✅ MySQL 连接成功!' AS result;" 2>nul
docker compose exec redis redis-cli ping 2>nul

echo.
echo ╔══════════════════════════════════════════════════════╗
echo ║           🎉 Docker 环境配置全部完成!               ║
echo ╠══════════════════════════════════════════════════════╣
echo ║                                                      ║
echo ║   MySQL:   localhost:3306  (root/root123)            ║
echo ║   Redis:   localhost:6379                           ║
echo ║   Server:  localhost:8900                           ║
echo ║                                                      ║
echo ║   进入容器: docker exec -it chatroom-dev bash       ║
echo ║   编译项目: dev.bat build-project                    ║
echo ║   运行程序: dev.bat run                             ║
echo ║                                                      ║
echo ║   VSCode/Trae 连接:                                 ║
echo ║   打开 chatroom-server 目录                          ║
echo ║   F1 → Dev Containers: Reopen in Container          ║
echo ╚══════════════════════════════════════════════════════╝
echo.

pause
