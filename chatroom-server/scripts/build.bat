@echo off
chcp 65001 >nul
echo ========================================
echo  C++ 直播项目 - 构建脚本
echo ========================================
echo.

set VCPKG_DIR=D:\vcpkg
set VS_PATH=D:\vs2019\1IDE
set CMAKE_EXE=%VS_PATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe
set BUILD_DIR=build

:: 初始化 VS 环境
call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

if not exist "%VCPKG_DIR%\vcpkg.exe" (
    echo [错误] 未找到 vcpkg！请先运行 setup_vcpkg.bat
    pause
    exit /b 1
)

if not exist "%CMAKE_EXE%" (
    echo [错误] 未找到 CMake！请确认 Visual Studio 已正确安装
    pause
    exit /b 1
)

echo [步骤 1/4] 创建构建目录...
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

echo.
echo [步骤 2/4] 运行 CMake 配置...
"%CMAKE_EXE%" .. -G "Visual Studio 16 2019" -A x64 ^
    -DCMAKE_TOOLCHAIN_FILE=%VCPKG_DIR%\scripts\buildsystems\vcpkg.cmake ^
    -DBUILD_TESTS=ON
if %errorLevel% neq 0 (
    echo [错误] CMake 配置失败！
    cd ..
    pause
    exit /b 1
)

echo.
echo [步骤 3/4] 编译项目（Release 模式）...
"%CMAKE_EXE%" --build . --config Release --parallel 8
if %errorLevel% neq 0 (
    echo [警告] Release 编译可能失败，尝试 Debug 模式...
    "%CMAKE_EXE%" --build . --config Debug --parallel 8
)

echo.
echo [步骤 4/4] 编译完成！
echo ========================================
echo  输出目录: %BUILD_DIR%\Release\
echo  可执行文件: chatroom_server.exe
echo ========================================

cd ..
pause
