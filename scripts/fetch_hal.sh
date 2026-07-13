#!/usr/bin/env bash
# =============================================================================
#  fetch_hal.sh —— 获取 STM32CubeF1 的 HAL 与 CMSIS（约 27MB，而非整仓 >1GB）
#
#  官方 STM32CubeF1 仓库把 HAL Driver 与 CMSIS Device 做成了「嵌套子模块」，
#  且仓库里还含庞大的 DSP/NN/例程。本脚本：
#    1) 浅克隆主仓（blobless + 不检出）；
#    2) sparse 只取 CMSIS Core 头 + 两个子模块挂载点（排除 DSP/NN/BSP 等大件）；
#    3) 只初始化我们真正需要的两个子模块：HAL Driver 与 CMSIS Device F1。
#
#  CI 与 scripts/bootstrap.* 共用本脚本；已存在则跳过。
# =============================================================================
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DIR="$ROOT/core/STM32CubeF1"
URL="https://github.com/STMicroelectronics/STM32CubeF1.git"

if [ -f "$DIR/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c" ]; then
    echo "[fetch_hal] HAL 已存在，跳过：$DIR"
    exit 0
fi

echo "[fetch_hal] 浅克隆主仓（blobless, 不检出）..."
rm -rf "$DIR"
git clone --depth 1 --filter=blob:none --no-checkout "$URL" "$DIR"

echo "[fetch_hal] sparse：只取 CMSIS Core 头与两个子模块挂载点..."
git -C "$DIR" sparse-checkout init --cone
git -C "$DIR" sparse-checkout set \
    Drivers/CMSIS/Include \
    Drivers/CMSIS/Core \
    Drivers/STM32F1xx_HAL_Driver \
    Drivers/CMSIS/Device/ST/STM32F1xx
git -C "$DIR" checkout

echo "[fetch_hal] 初始化 HAL Driver 与 CMSIS Device 子模块（浅）..."
git -C "$DIR" submodule update --init --depth 1 \
    Drivers/STM32F1xx_HAL_Driver \
    Drivers/CMSIS/Device/ST/STM32F1xx

echo "[fetch_hal] 完成（约 27MB）：$DIR"
