@echo off
chcp 65001 >nul 2>&1
echo ========================================
echo   Chatroom Server - 一键启动脚本
echo ========================================
echo.

cd /d "%~dp0..\docker"

echo [1/5] 检查Docker容器状态...
docker compose ps | findstr "chatroom-dev" >nul 2>&1
if %errorlevel% neq 0 (
    echo [INFO] 容器未启动，正在启动...
    docker compose up -d
    timeout /t 3 /nobreak >nul
) else (
    echo [INFO] 容器已运行
)

echo.
echo [2/5] 进入Docker容器并编译项目...
docker exec chatroom-dev bash -c "cd /workspace/backend-server && mkdir -p build logs && cd build && if [ ! -f Makefile ]; then cmake .. -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake; fi && cmake --build ."

if %errorlevel% neq 0 (
    echo [ERROR] 编译失败！请检查错误信息。
    pause
    exit /b 1
)

echo.
echo [3/5] 运行单元测试...
docker exec chatroom-dev bash -c "cd /workspace/backend-server/build && ./chatroom-tests"

if %errorlevel% neq 0 (
    echo [WARNING] 部分测试失败，但可以继续运行服务器
) else (
    echo [SUCCESS] 所有测试通过！
)

echo.
echo [4/5] 准备启动服务器...
echo ========================================
echo   服务器将在 0.0.0.0:8900 启动
echo   按 Ctrl+C 可停止服务器
echo ========================================
echo.

echo [5/5] 启动服务器...
docker exec -it chatroom-dev bash -c "cd /workspace/backend-server/build && ./chatroom-server ../config/app.json"

echo.
echo [INFO] 服务器已停止
pause
