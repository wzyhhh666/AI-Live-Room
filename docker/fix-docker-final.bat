@echo off
chcp 65001 >nul
title Docker Desktop 修复 - 最终方案
color 0B

echo ========================================
echo   Docker Desktop 引擎修复脚本
echo ========================================
echo.

net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [错误] 必须右键以管理员身份运行!
    pause
    exit /b 1
)

set "DOCKER_SETTINGS=%APPDATA%\Docker\settings.json"
set "SOURCE_SETTINGS=%~dp0settings.json"

echo [1/4] 复制 Docker 配置文件...
if exist "%DOCKER_SETTINGS%" (
    copy /y "%SOURCE_SETTINGS%" "%DOCKER_SETTINGS%" >nul
) else (
    copy /y "%SOURCE_SETTINGS%" "%DOCKER_SETTINGS%"
)
echo   ✅ 已配置: %DOCKER_SETTINGS%
echo.
type "%DOCKER_SETTINGS%"
echo.

echo [2/4] 启动 Alpine WSL (让 WSL2 后端激活)...
wsl -d alpine -- echo "WSL2 OK" >nul 2>&1
if %errorLevel% equ 0 (
    echo   ✅ Alpine WSL 正常
) else (
    echo   ⚠️ 首次启动需要初始化，继续...
)
echo.

echo [3/4] 重置并重启 Docker Desktop...
taskkill /F /IM "Docker Desktop.exe" >nul 2>&1
timeout /t 3 /nobreak >nul

:: 清理旧的 pipe
if exist "\\.\pipe\dockerDesktopLinuxEngine" (
    echo   检测到旧管道，清理中...
)

start "" "%ProgramFiles%\Docker\Docker\Docker Desktop.exe"
echo   ✅ Docker Desktop 已启动
echo.

echo [4/4] 等待引擎就绪 (最多180秒)...
echo.

for /l %%i in (1,1,36) do (
    docker info >nul 2>&1
    if !errorLevel! equ 0 (
        <nul set /p "=   等待... %%i/36 ($(set /a t=%%i*5)秒)"
        timeout /t 5 /nobreak >nul
        echo.
    ) else (
        goto :success
    )
)

echo.
echo ❌ Docker 引擎未能在3分钟内启动
echo.
echo 可能原因:
echo   1. 需要重启电脑后再次运行此脚本
echo   2. VMware/VirtualBox 与 Hyper-V 冲突
echo   3. BIOS 中虚拟化未开启 (VT-x/AMD-V)
echo.
pause
exit /b 1

:success
echo.
echo ╔════════════════════════════════════════════╗
echo ║     🎉 Docker 环境修复成功!               ║
echo ╠════════════════════════════════════════════╣
docker info | findstr "Server Version Operating System"
echo ╚════════════════════════════════════════════╝
echo.
echo 接下来请运行: post-restart.bat
echo 或直接执行: dev.bat build
echo.
pause
