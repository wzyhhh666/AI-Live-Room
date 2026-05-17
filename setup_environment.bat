@echo off
chcp 65001 >nul
echo ========================================
echo  C++ 直播项目 - Windows 开发环境配置脚本
echo ========================================
echo.

:: 检查管理员权限
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [错误] 请以管理员身份运行此脚本！
    pause
    exit /b 1
)

echo [步骤 1/6] 安装 Visual Studio 2022 Build Tools...
echo.
winget install Microsoft.VisualStudio.2022.BuildTools --override "--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended" --accept-package-agreements --accept-source-agreements
if %errorLevel% neq 0 (
    echo [警告] Visual Studio 安装可能失败，请检查...
)
echo.

echo [步骤 2/6] 安装 CMake...
winget install Kitware.CMake --accept-package-agreements --accept-source-agreements
if %errorLevel% neq 0 (
    echo [警告] CMake 安装可能失败...
)
echo.

echo [步骤 3/6] 安装 Git（vcpkg 需要）...
winget install Git.Git --accept-package-agreements --accept-source-agreements
if %errorLevel% neq 0 (
    echo [警告] Git 安装可能失败...
)
echo.

echo [步骤 4/6] 安装 Python3（部分工具需要）...
winget install Python.Python.3 --accept-package-agreements --accept-source-agreements
if %errorLevel% neq 0 (
    echo [警告] Python 安装可能失败...
)
echo.

echo ========================================
echo  基础工具安装完成！
echo  接下来需要重启终端并运行 setup_vcpkg.bat
echo ========================================
pause
