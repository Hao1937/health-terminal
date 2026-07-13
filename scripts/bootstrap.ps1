# =============================================================================
#  bootstrap.ps1 —— Windows 一键就绪：获取 HAL + 检查工具链 + 编译自检
#  用法（PowerShell）：  ./scripts/bootstrap.ps1
# =============================================================================
$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "..")

Write-Host "==> 1/3 获取 STM32CubeF1 HAL（只拉 Drivers）"
$halDir = "core/STM32CubeF1"
if (Test-Path "$halDir/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c") {
  Write-Host "  HAL 已存在，跳过"
} else {
  if (Test-Path $halDir) { Remove-Item -Recurse -Force $halDir }
  git clone --depth 1 --filter=blob:none --sparse https://github.com/STMicroelectronics/STM32CubeF1.git $halDir
  git -C $halDir sparse-checkout set Drivers
}

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
