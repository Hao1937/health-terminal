#!/usr/bin/env bash
# =============================================================================
#  fetch_hal.sh —— 获取 STM32CubeF1 HAL（只拉 Drivers 目录，约数十 MB）
#  用 partial + sparse clone，避免拉下整个含例程的巨大仓库。
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

echo "[fetch_hal] 获取 STM32CubeF1 的 Drivers（sparse, depth=1）..."
rm -rf "$DIR"
git clone --depth 1 --filter=blob:none --sparse "$URL" "$DIR"
git -C "$DIR" sparse-checkout set Drivers
echo "[fetch_hal] 完成：$DIR/Drivers"
