@echo off
chcp 65001 >nul
echo ========================================
echo  一键环境完成脚本 (Header-Only 模式)
echo  无需 vcpkg，无需编译第三方库
echo ========================================
echo.

set PROJECT_DIR=d:\ai直播项目\chatroom-server
set THIRD_PARTY=%PROJECT_DIR%\third_party

:: 创建 third_party 目录
if not exist "%THIRD_PARTY%" mkdir "%THIRD_PARTY%"

echo [步骤 1/5] 下载 spdlog (header-only)...
if not exist "%THIRD_PARTY%\spdlog" (
    git clone --depth 1 --branch v1.14.1 https://gitee.com/mirrors/spdlog.git "%THIRD_PARTY%\spdlog"
    if %errorLevel% neq 0 echo [警告] spdlog 下载失败，尝试 GitHub...
    if not exist "%THIRD_PARTY%\spdlog" git clone --depth 1 --branch v1.14.1 https://github.com/gabime/spdlog.git "%THIRD_PARTY%\spdlog"
) else (
    echo    已存在，跳过
)

echo.
echo [步骤 2/5] 下载 nlohmann/json (header-only)...
if not exist "%THIRD_PARTY%\nlohmann-json" (
    mkdir "%THIRD_PARTY%\nlohmann-json"
    curl -L -o "%THIRD_PARTY%\nlohmann-json\json.hpp" https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp
    if %errorLevel% neq 0 (
        curl -L -o "%THIRD_PARTY%\nlohmann-json\json.hpp" https://gitee.com/mirrors/nlohmann_json/raw/develop/single_include/nlohmann/json.hpp
    )
) else (
    echo    已存在，跳过
)

echo.
echo [步骤 3/5] 下载 fmt (header-only)...
if not exist "%THIRD_PARTY%\fmt" (
    git clone --depth 1 --branch 11.0.2 https://gitee.com/mirrors/fmt.git "%THIRD_PARTY%\fmt"
    if %errorLevel% neq 0 git clone --depth 1 --branch 11.0.2 https://github.com/fmtlib/fmt.git "%THIRD_PARTY%\fmt"
) else (
    echo    已存在，跳过
)

echo.
echo [步骤 4/5] 验证依赖库...
set MISSING=0
for %%L in (spdlog nlohmann-json fmt) do (
    if exist "%THIRD_PARTY%\%%L" (
        echo    ✅ %%L
    ) else (
        echo    ❌ %%L 缺失
        set /a MISSING+=1
    )
)

echo.
if %MISSING% gtr 0 (
    echo [警告] 部分依赖缺失，项目可能无法完整编译
) else (
    echo ✅ 所有基础依赖已就绪！
)

echo.
echo [步骤 5/5] 配置构建环境...
cd /d "%PROJECT_DIR%"
if not exist build mkdir build

echo.
echo ========================================
echo  🎉 环境配置完成！
echo ========================================
echo.
echo  现在可以运行以下命令编译项目:
echo.
echo    cd %PROJECT_DIR%\build
echo    cmake .. -G "Visual Studio 16 2019" -A x64
echo    cmake --build . --config Release
echo.
echo  或直接运行:
echo    scripts\build.bat
echo.
pause
