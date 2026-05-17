@echo off
chcp 65001 >nul
echo ========================================
echo  安装和配置 vcpkg 包管理器
echo ========================================
echo.

:: 设置 vcpkg 安装位置
set VCPKG_DIR=D:\vcpkg

:: 检查是否已存在
if exist "%VCPKG_DIR%\.git" (
    echo [信息] vcpkg 已存在于 %VCPKG_DIR%
    echo 正在更新...
    cd /d "%VCPKG_DIR%"
    git pull
) else (
    echo [步骤 1/3] 克隆 vcpkg 仓库...
    git clone https://github.com/microsoft/vcpkg.git "%VCPKG_DIR%"
    if %errorLevel% neq 0 (
        echo [错误] vcpkg 克隆失败！
        pause
        exit /b 1
    )
    cd /d "%VCPKG_DIR%"
)

echo.
echo [步骤 2/3] 引导 vcpkg...
call bootstrap-vcpkg.bat -disableMetrics
if %errorLevel% neq 0 (
    echo [错误] vcpkg 引导失败！
    pause
    exit /b 1
)

echo.
echo [步骤 3/3] 集成 vcpkg 到系统...
call vcpkg integrate install
if %errorLevel% neq 0 (
    echo [警告] vcpkg 集成可能失败...
)

echo.
echo ========================================
echo  vcpkg 安装完成！
echo  安装路径: %VCPKG_DIR%
echo  接下来请运行 install_dependencies.bat
echo ========================================
pause
