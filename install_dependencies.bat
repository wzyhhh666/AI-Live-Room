@echo off
chcp 65001 >nul
echo ========================================
echo  安装项目第三方依赖库
echo ========================================
echo.

set VCPKG_DIR=D:\vcpkg

:: 检查 vcpkg 是否存在
if not exist "%VCPKG_DIR%\vcpkg.exe" (
    echo [错误] 未找到 vcpkg！请先运行 setup_vcpkg.bat
    pause
    exit /b 1
)

cd /d "%VCPKG_DIR%"

echo [依赖 1/8] 安装 spdlog（日志库）...
call vcpkg install spdlog:x64-windows
if %errorLevel% neq 0 echo [警告] spdlog 安装失败

echo.
echo [依赖 2/8] 安装 fmt（格式化库）...
call vcpkg install fmt:x64-windows
if %errorLevel% neq 0 echo [警告] fmt 安装失败

echo.
echo [依赖 3/8] 安装 protobuf（协议序列化）...
call vcpkg install protobuf:x64-windows
if %errorLevel% neq 0 echo [警告] protobuf 安装失败

echo.
echo [依赖 4/8] 安装 gtest（单元测试框架）...
call vcpkg install gtest:x64-windows
if %errorLevel% neq 0 echo [警告] gtest 安装失败

echo.
echo [依赖 5/8] 安装 yaml-cpp（YAML配置解析）...
call vcpkg install yaml-cpp:x64-windows
if %errorLevel% neq 0 echo [警告] yaml-cpp 安装失败

echo.
echo [依赖 6/8] 安装 nlohmann-json（JSON处理）...
call vcpkg install nlohmann-json:x64-windows
if %errorLevel% neq 0 echo [警告] nlohmann-json 安装失败

echo.
echo [依赖 7/8] 安装 hiredis（Redis客户端）...
call vcpkg install hiredis:x64-windows
if %errorLevel% neq 0 echo [警告] hiredis 安装失败

echo.
echo [依赖 8/8] 安装 mysql-connector-cpp（MySQL客户端）...
call vcpkg install mysql-connector-cpp:x64-windows
if %errorLevel% neq 0 echo [警告] mysql-connector-cpp 安装失败

echo.
echo ========================================
echo  所有依赖库安装完成！
echo  已安装的库：
echo    - spdlog (日志)
echo    - fmt (格式化)
echo    - protobuf (序列化)
echo    - gtest (测试)
echo    - yaml-cpp (YAML)
echo    - nlohmann-json (JSON)
echo    - hiredis (Redis)
echo    - mysql-connector-cpp (MySQL)
echo ========================================
pause
