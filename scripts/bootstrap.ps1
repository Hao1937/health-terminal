# =============================================================================
#  bootstrap.ps1 —— Windows 一键就绪：拉 submodule + 检查工具链 + 编译自检
#  用法（PowerShell）：  ./scripts/bootstrap.ps1
# =============================================================================
$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "..")

Write-Host "==> 1/3 拉取 STM32CubeF1 submodule"
git submodule update --init --recursive

Write-Host "==> 2/3 检查工具链"
function Need($name) {
  if (Get-Command $name -ErrorAction SilentlyContinue) { Write-Host "  [OK] $name" }
  else { Write-Host "  [缺失] $name —— 见 docs/开发环境搭建.md" -ForegroundColor Yellow }
}
Need cmake
Need ninja
Need arm-none-eabi-gcc
Need openocd

Write-Host "==> 3/3 host 单元测试自检"
cmake --preset host
cmake --build --preset host
ctest --preset host

Write-Host "==> 完成。烧录固件见 docs/开发环境搭建.md（cmake --preset stm32）"
