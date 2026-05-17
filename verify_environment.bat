@echo off
chcp 65001 >nul
echo ========================================
echo  环境验证脚本
echo ========================================
echo.

set VS_PATH=D:\vs2019\1IDE
set VCPKG_DIR=D:\vcpkg
set CMAKE_EXE=%VS_PATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe

echo [检查 1/5] Visual Studio 2019...
if exist "%VS_PATH%\VC\Tools\MSVC" (
    echo     ✅ 找到 MSVC 编译器
    for /d %%i in ("%VS_PATH%\VC\Tools\MSVC\*") do echo        版本: %%~nxi
) else (
    echo     ❌ 未找到 Visual Studio
)

echo.
echo [检查 2/5] CMake...
if exist "%CMAKE_EXE%" (
    echo     ✅ CMake 已安装
    "%CMAKE_EXE%" --version | findstr /C:"cmake version"
) else (
    echo     ❌ CMake 未安装
)

echo.
echo [检查 3/5] Git...
where git >nul 2>&1
if %errorLevel% equ 0 (
    echo     ✅ Git 已安装
    git --version
) else (
    echo     ❌ Git 未安装
)

echo.
echo [检查 4/5] vcpkg...
if exist "%VCPKG_DIR%\vcpkg.exe" (
    echo     ✅ vcpkg 已安装
    "%VCPKG_DIR%\vcpkg.exe" version
) else (
    echo     ⏳ vcpkg 正在安装或未安装
    if exist "%VCPKG_DIR%\.git" (
        echo        仓库已克隆，需要执行 bootstrap-vcpkg.bat
    )
)

echo.
echo [检查 5/5] 项目目录结构...
if exist "chatroom-server\CMakeLists.txt" (
    echo     ✅ 项目结构已创建
) else (
    echo     ❌ 项目结构缺失
)

echo.
echo ========================================
echo  验证完成！
echo ========================================
pause
