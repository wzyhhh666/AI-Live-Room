@echo off
chcp 65001 >nul
setlocal

set "PROJECT_DIR=%~dp0.."
set "DOCKER_DIR=%PROJECT_DIR%\docker"

if "%1"=="" goto :help

if "%1"=="build" goto :build
if "%1"=="up" goto :up
if "%1"=="down" goto :down
if "%1"=="shell" goto :shell
if "%1"=="build-project" goto :build_project
if "%1"=="run" goto :run
if "%1"=="logs" goto :logs
if "%1"=="clean" goto :clean
if "%1"=="status" goto :status
goto :help

:build
echo ============================================
echo  [1/4] 构建 Docker 开发环境镜像...
echo ============================================
cd /d "%DOCKER_DIR%"
docker compose build chatroom-server
if %errorlevel% neq 0 (
    echo ❌ 构建失败!
    exit /b 1
)
echo.
echo ✅ 镜像构建完成!
goto :eof

:up
echo ============================================
echo  [2/4] 启动所有服务...
echo ============================================
cd /d "%DOCKER_DIR%"
docker compose up -d mysql redis
echo 等待 MySQL 就绪 (约10秒)...
timeout /t 10 /nobreak >nul
docker compose up -d chatroom-server
echo.
docker compose ps
echo.
echo ✅ 服务已启动!
echo.
echo   MySQL:   localhost:3306 (root/root123)
echo   Redis:   localhost:6379
echo   Server:  localhost:8900
echo.
echo 进入容器: docker exec -it chatroom-dev bash
goto :eof

:down
echo [停止所有服务...]
cd /d "%DOCKER_DIR%"
docker compose down
echo ✅ 已停止
goto :eof

:shell
cd /d "%DOCKER_DIR%"
docker compose exec chatroom-server bash
goto :eof

:build_project
echo [在容器内编译项目...]
cd /d "%DOCKER_DIR%"
docker compose exec chatroom-server bash -c "cd /workspace/chatroom-server && mkdir -p build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && cmake --build . --parallel $(nproc) && echo '' && echo '✅ 编译完成!'"
goto :eof

:run
echo [运行服务器...]
cd /d "%DOCKER_DIR%"
docker compose exec chatroom-server bash -c "cd /workspace/chatroom-server && ./build/chatroom_server"
goto :eof

:logs
cd /d "%DOCKER_DIR%"
docker compose logs -f chatroom-server
goto :eof

:clean
echo [清理构建产物和容器...]
cd /d "%DOCKER_DIR%"
docker compose down -v --rmi local
if exist "%PROJECT_DIR%\chatroom-server\build" rmdir /s /q "%PROJECT_DIR%\chatroom-server\build"
echo ✅ 清理完成
goto :eof

:status
cd /d "%DOCKER_DIR%"
docker compose ps
echo.
docker compose exec chatroom-server bash -c "gcc --version | head -1 && cmake --version | head -1"
goto :eof

:help
echo ============================================
echo  C++ 直播项目 - Docker 开发环境
echo ============================================
echo.
echo 用法: %0 {命令}
echo.
echo 命令:
echo   build         构建开发环境镜像 (首次使用)
echo   up            启动全部服务 (MySQL + Redis + 容器)
echo   down          停止所有服务
echo   shell         进入容器终端
echo   build-project 在容器内编译项目
echo   run           运行服务器
echo   logs          查看日志
echo   status        查看状态
echo   clean         清理所有数据(慎用!)
echo.
echo 快速开始:
echo   %0 build       首次: 构建镜像 (~5-10分钟)
echo   %0 up          启动服务
echo   %0 shell       进入容器写代码
echo.
echo VSCode/Trae 连接方式:
echo   打开 chatroom-server 目录 → F1 → Dev Containers: Reopen in Container
goto :eof
