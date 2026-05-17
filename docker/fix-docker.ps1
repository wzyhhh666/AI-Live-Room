# Docker 环境修复脚本
# 使用方法: 右键 → 以管理员身份运行 PowerShell → 输入: powershell -ExecutionPolicy Bypass -File "d:\ai直播项目\docker\fix-docker.ps1"

Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  Docker 修复工具 (PowerShell 版)" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

# 检查管理员权限
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Host "[错误] 请以管理员身份运行!" -ForegroundColor Red
    Read-Host "按回车键退出"
    exit 1
}

Write-Host "[1/3] 安装 WSL Linux 内核..." -ForegroundColor Yellow

# 下载并安装 WSL 内核
$kernelMsi = "$env:TEMP\wsl_update_x64.msi"
if (-not (Test-Path $kernelMsi)) {
    Write-Host "  下载 WSL 内核..." -ForegroundColor Gray
    Invoke-WebRequest -Uri "https://wslstorestorage.blob.core.windows.net/wslblob/wsl_update_x64.msi" -OutFile $kernelMsi -UseBasicParsing
}
if (Test-Path $kernelMsi) {
    Start-Process msiexec.exe -ArgumentList "/i",$kernelMsi,"/quiet","/norestart" -Wait -NoNewWindow
    Write-Host "  WSL 内核: OK" -ForegroundColor Green
} else {
    Write-Host "  WSL 内核: 跳过" -ForegroundColor Gray
}

Write-Host ""
Write-Host "[2/3] 安装 Ubuntu 发行版..." -ForegroundColor Yellow

# 检查是否已安装
$installed = wsl --list -q 2>$null | Select-String "Ubuntu"
if ($installed) {
    Write-Host "  Ubuntu: 已存在" -ForegroundColor Green
} else {
    Write-Host "  正在安装 Ubuntu (约200MB)..." -ForegroundColor Gray
    
    # 方式1: winget
    $wingetResult = winget install Canonical.Ubuntu.LTS --accept-package-agreements --accept-source-agreements 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  Ubuntu: 安装成功 (winget)" -ForegroundColor Green
    } else {
        # 方式2: wsl install
        Write-Host "  尝试 wsl --install..." -ForegroundColor Gray
        wsl --install -d Ubuntu --web-download --no-launch 2>$null | Out-Null
        
        # 检查结果
        Start-Sleep -Seconds 5
        $installed = Get-ChildItem "$env:LOCALAPPDATA\Packages" -Filter "*Ubuntu*" -Directory -ErrorAction SilentlyContinue
        if ($installed) {
            Write-Host "  Ubuntu: 安装成功" -ForegroundColor Green
        } else {
            Write-Host "  Ubuntu: 自动安装失败" -ForegroundColor Red
            Write-Host ""
            Write-Host "请手动执行以下命令:" -ForegroundColor Yellow
            Write-Host "  1. 打开 PowerShell (管理员)" -ForegroundColor White
            Write-Host "  2. 运行: wsl --install -d Ubuntu" -ForegroundColor White
            Write-Host "  3. 重启电脑" -ForegroundColor White
        }
    }
}

Write-Host ""
Write-Host "[3/3] 重启 Docker Desktop..." -ForegroundColor Yellow

Stop-Process -Name "Docker Desktop" -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 3
Start-Process "$env:ProgramFiles\Docker\Docker\Docker Desktop.exe"
Write-Host "  Docker Desktop 已重启" -ForegroundColor Green

Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  等待 Docker 引擎就绪 (最多3分钟)..." -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

$env:PATH = "$env:ProgramFiles\Docker\Docker\resources\bin;$env:PATH"
$maxWait = 180
for ($i = 1; $i -le $maxWait; $i++) {
    $result = docker info 2>&1
    if ($LASTEXITCODE -eq 0 -and $result -match "Server Version") {
        Write-Host "`n============================================" -ForegroundColor Green
        Write-Host "  🎉 Docker 环境修复成功!" -ForegroundColor Green
        Write-Host "============================================" -ForegroundColor Green
        docker version | Select-String "Version"
        Write-Host ""
        Write-Host "下一步: 运行 docker\post-restart.bat" -ForegroundColor Yellow
        Read-Host "按回车键退出"
        exit 0
    }
    
    if ($i % 10 -eq 0) {
        Write-Host "  已等待 $i 秒..." -ForegroundColor Gray
    }
    Start-Sleep -Seconds 1
}

Write-Host "`n❌ Docker 引擎未能在3分钟内启动" -ForegroundColor Red
Write-Host "可能需要重启电脑后再次运行此脚本" -ForegroundColor Yellow
Read-Host "按回车键退出"
