@echo off
chcp 65001 >nul
color 0E
title Docker 一键修复 - 需要管理员权限

echo ╔══════════════════════════════════════════╗
echo ║   Docker 修复工具                           ║
echo ║   正在安装 WSL Ubuntu 并启动 Docker...      ║
echo ╚══════════════════════════════════════════╝
echo.

net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [错误] 请右键此文件 → 以管理员身份运行!
    pause
    exit /b 1
)

set "PATH=%ProgramFiles%\Docker\Docker\resources\bin;%PATH%"

echo [1/3] 安装 WSL Linux 内核...
if not exist "%TEMP%\wsl_update_x64.msi" (
    echo   下载中 (约16MB)...
    powershell -Command "Invoke-WebRequest -Uri 'https://wslstorestorage.blob.core.windows.net/wslblob/wsl_update_x64.msi' -OutFile '%TEMP%\wsl_update_x64.msi' -UseBasicParsing"
)
if exist "%TEMP%\wsl_update_x64.msi" (
    msiexec /i "%TEMP%\wsl_update_x64.msi" /quiet /norestart
    echo   ✅ 内核已安装
) else (
    echo   ⚠️ 跳过内核安装
)

echo.
echo [2/3] 安装 Ubuntu 发行版 (约200MB，需2-5分钟)...
wsl --list -q 2>nul | findstr /i "Ubuntu" >nul 2>&1
if %errorLevel% neq 0 (
    wsl --install -d Ubuntu --web-download --no-launch
    if %errorLevel% equ 0 (
        echo   ✅ Ubuntu 已安装
    ) else (
        echo   ⚠️ 自动安装失败，请手动在 PowerShell(管理员) 中执行:
        echo     wsl --install -d Ubuntu --web-download --no-launch
    )
) else (
    echo   ✅ Ubuntu 已存在
)

echo.
echo [3/3] 重启 Docker Desktop...
taskkill /F /IM "Docker Desktop.exe" >nul 2>&1
timeout /t 3 /nobreak >nul
start "" "%ProgramFiles%\Docker\Docker\Docker Desktop.exe"
echo   Docker Desktop 已重启

echo.
echo ========================================
echo  等待 Docker 引擎就绪 (最多3分钟)...
echo ========================================

for /l %%i in (1,1,180) do (
    docker info >nul 2>&1
    if !errorLevel! equ 0 (
        if %%i==1 (echo   等待中...) else (<nul set /p "=   %%i 秒" & echo .)
        timeout /t 1 /nobreak >nul
    ) else (
        goto :ok
    )
)
echo.
echo ❌ Docker 引擎未能在 3 分钟内启动
echo 可能需要重启电脑后再次运行此脚本
pause
exit /b 1

:ok
echo.
echo ╔════════════════════════════════════════════╗
echo ║         🎉 Docker 环境修复成功!            ║
echo ╠════════════════════════════════════════════╣
docker version | findstr "Version"
echo ╚════════════════════════════════════════════╝
echo.
echo 接下来请运行: docker\post-restart.bat
echo 或直接执行: cd docker ^& dev.bat build
echo.
pause
