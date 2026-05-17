@echo off
chcp 65001 >nul
title C++ 直播项目 - Docker 环境一键配置
color 0A

echo ╔══════════════════════════════════════════════════════╗
echo ║     Docker 开发环境 - 一键安装脚本                    ║
echo ║     请确保以【管理员身份】运行此脚本!                 ║
echo ╚══════════════════════════════════════════════════════╝
echo.

:: 检查管理员权限
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [错误] 请右键此脚本 → 以管理员身份运行!
    pause
    exit /b 1
)
echo [✅] 管理员权限确认
echo.

:: ====== 第 1 步: 启用 WSL ======
echo ========================================
echo [1/5] 启用 WSL (Windows Subsystem for Linux)...
echo ========================================
dism.exe /online /enable-feature /featurename:Microsoft-Windows-Subsystem-Linux /all /norestart
if %errorLevel% neq 0 (
    echo [警告] WSL 启用可能失败，继续尝试...
)

:: ====== 第 2 步: 启用虚拟机平台 ======
echo.
echo [2/5] 启用虚拟机平台 (Virtual Machine Platform)...
dism.exe /online /enable-feature /featurename:VirtualMachinePlatform /all /norestart
if %errorLevel% neq 0 (
    echo [警告] 虚拟机平台启用可能失败...
)

:: ====== 第 3 步: 设置 WSL2 为默认版本 ======
echo.
echo [3/5] 设置 WSL2 为默认版本...
wsl --set-default-version 2
if %errorLevel% neq 0 (
    echo [提示] 如果 wsl 命令不存在，将在重启后生效
)

:: ====== 第 4 步: 安装 Docker Desktop ======
echo.
echo ========================================
echo [4/5] 安装 Docker Desktop for Windows...
echo ========================================

:: 检查是否已安装
where docker >nul 2>&1
if %errorLevel% equ 0 (
    echo [信息] Docker Desktop 已安装，跳过下载
) else (
    echo 正在通过 winget 下载安装 Docker Desktop...
    winget install Docker.DockerDesktop --accept-package-agreements --accept-source-agreements
    
    if %errorLevel% neq 0 (
        echo [备选] winget 失败，尝试直接下载...
        
        :: 下载 Docker Desktop Installer
        echo 正在从官网下载 Docker Desktop...
        powershell -Command "Invoke-WebRequest -Uri 'https://desktop.docker.com/win/main/amd64/Docker%20Desktop%20Installer.exe' -OutFile '%TEMP%\DockerInstaller.exe' -UseBasicParsing"
        
        if exist "%TEMP%\DockerInstaller.exe" (
            echo 正在静默安装 Docker Desktop...
            start /wait "" "%TEMP%\DockerInstaller.exe" install --quiet --accept-license
            echo [✅] Docker Desktop 安装完成
        ) else (
            echo [❌] 下载失败! 请手动下载:
            echo      https://www.docker.com/products/docker-desktop/
            pause
            exit /b 1
        )
    )
)

:: ====== 第 5 步: 完成 ======
echo.
echo ========================================
echo [5/5] 配置完成!
echo ========================================
echo.
echo ✅ 已完成的操作:
echo   ① WSL (Linux 子系统) 已启用
echo   ② 虚拟机平台已启用
echo   ③ WSL2 设为默认版本
echo   ④ Docker Desktop 已安装
echo.
echo ⚠️ 接下来你需要:
echo   1. 重启电脑 (必须!)
echo   2. 重启后启动 Docker Desktop
echo   3. 运行 docker\dev.bat build 构建开发环境
echo.
set /p RESTART="是否现在重启电脑? (Y/N): "
if /i "%RESTART%"=="Y" (
    shutdown /r /t 10 /c "重启以完成 Docker 环境配置"
) else (
    echo 请稍后手动重启电脑。
)

pause
