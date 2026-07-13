# =============================================================================
#  bootstrap.ps1 —— Windows 一键就绪：获取 HAL + 检查工具链 + 编译自检
#  用法（PowerShell）：  ./scripts/bootstrap.ps1
# =============================================================================
$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "..")

Write-Host "==> 1/3 获取 STM32CubeF1 的 HAL 与 CMSIS（约 27MB）"
$halDir = "core/STM32CubeF1"
$halUrl = "https://github.com/STMicroelectronics/STM32CubeF1.git"
if (Test-Path "$halDir/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c") {
  Write-Host "  HAL 已存在，跳过"
} else {
  if (Test-Path $halDir) { Remove-Item -Recurse -Force $halDir }
  # 主仓的 HAL Driver 与 CMSIS Device 是嵌套子模块；浅克隆 + sparse + 只初始化这两个
  git clone --depth 1 --filter=blob:none --no-checkout $halUrl $halDir
  git -C $halDir sparse-checkout init --cone
  git -C $halDir sparse-checkout set Drivers/CMSIS/Include Drivers/CMSIS/Core Drivers/STM32F1xx_HAL_Driver Drivers/CMSIS/Device/ST/STM32F1xx
  git -C $halDir checkout
  git -C $halDir submodule update --init --depth 1 Drivers/STM32F1xx_HAL_Driver Drivers/CMSIS/Device/ST/STM32F1xx
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
