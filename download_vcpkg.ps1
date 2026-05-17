$vcpkgExeUrl = "https://gh.api.99988866.xyz/https://github.com/microsoft/vcpkg-tool/releases/download/2024-04-23/vcpkg.exe"
$downloadPath = "D:\vcpkg\vcpkg.exe"

Write-Host "正在通过代理下载 vcpkg.exe..."
[System.Net.ServicePointManager]::SecurityProtocol = [System.Net.SecurityProtocolType]::Tls12

$wc = New-Object System.Net.WebClient
$wc.DownloadFile($vcpkgExeUrl, $downloadPath)

if (Test-Path $downloadPath) {
    $size = [math]::Round((Get-Item $downloadPath).Length / 1MB, 2)
    Write-Host "SUCCESS: vcpkg.exe downloaded! Size: $size MB"
} else {
    Write-Host "FAILED: Download failed"
}
