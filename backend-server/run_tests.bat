@echo off
chcp 65001 >nul 2>&1
echo ========================================
echo   Chatroom Server - 快速测试脚本
echo ========================================
echo.

cd /d "%~dp0..\docker"

echo [1/3] 检查Docker容器...
docker compose ps | findstr "chatroom-dev" >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Docker容器未启动！请先运行 run_server.bat
    pause
    exit /b 1
)

echo [2/3] 编译项目...
docker exec chatroom-dev bash -c "cd /workspace/backend-server && mkdir -p build logs && cd build && if [ ! -f Makefile ]; then cmake .. -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake; fi && cmake --build ."

if %errorlevel% neq 0 (
    echo [ERROR] 编译失败！
    pause
    exit /b 1
)

echo.
echo [3/3] 运行单元测试...
echo ----------------------------------------
docker exec chatroom-dev bash -c "cd /workspace/backend-server/build && ./chatroom-tests --gtest_color=yes"
echo ----------------------------------------

echo.
if %errorlevel% equ 0 (
    echo [SUCCESS] ✅ 所有测试通过！可以运行 run_server.bat 启动服务器
) else (
    echo [FAILED] ❌ 部分测试失败，请检查上方错误信息
)

pause
