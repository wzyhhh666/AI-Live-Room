@echo off
chcp 65001 >nul
echo ========================================
echo  vcpkg 安装脚本（国内镜像加速版）
echo ========================================
echo.

set VCPKG_DIR=D:\vcpkg

:: 清理可能的不完整下载
if exist "%VCPKG_DIR%" (
    echo [信息] 检测到旧的 vcpkg 目录，正在清理...
    rmdir /s /q "%VCPKG_DIR%"
)

echo.
echo [步骤 1/3] 从 Gitee 镜像克隆 vcpkg（推荐国内用户）...
echo 正在下载约 300MB 数据，请耐心等待...

git clone https://gitee.com/mirrors/vcpkg.git "%VCPKG_DIR%"
if %errorLevel% neq 0 (
    echo.
    echo [备选方案] Gitee 镜像失败，尝试 GitHub 源...
    git clone https://github.com/microsoft/vcpkg.git "%VCPKG_DIR%"
    if %errorLevel% neq 0 (
        echo [错误] vcpkg 克隆失败！请检查网络或尝试以下方法：
        echo        1. 使用 VPN 后重新运行此脚本
        echo        2. 手动下载: https://github.com/microsoft/vcpkg/archive/refs/heads/master.zip
        echo        3. 解压到 D:\vcpkg
        pause
        exit /b 1
    )
)

cd /d "%VCPKG_DIR%"

echo.
echo [步骤 2/3] 引导 vcpkg（编译可执行文件）...
call bootstrap-vcpkg.bat -disableMetrics
if %errorLevel% neq 0 (
    echo [错误] vcpkg 引导失败！
    pause
    exit /b 1
)

echo.
echo [步骤 3/3] 集成到全局系统...
call vcpkg integrate install
if %errorLevel% neq 0 (
    echo [警告] 集成可能失败，但不影响使用
)

echo.
echo ========================================
echo  ✅ vcpkg 安装成功！
echo  路径: %VCPKG_DIR%
echo  版本:
call vcpkg version
echo.
echo 下一步: 运行 install_dependencies.bat 安装依赖库
echo ========================================
pause
