#!/usr/bin/env bash
# =============================================================================
#  bootstrap.sh —— Linux / macOS 一键就绪：拉 submodule + 检查工具链 + 编译自检
# =============================================================================
set -e
cd "$(dirname "$0")/.."

echo "==> 1/3 拉取 STM32CubeF1 submodule"
git submodule update --init --recursive

echo "==> 2/3 检查工具链"
need() { command -v "$1" >/dev/null 2>&1 && echo "  [OK] $1" || echo "  [缺失] $1 —— 见 docs/开发环境搭建.md"; }
need cmake
need ninja
need arm-none-eabi-gcc
need openocd
need cc

echo "==> 3/3 host 单元测试自检"
cmake --preset host
cmake --build --preset host
ctest --preset host

echo "==> 完成。烧录固件见 docs/开发环境搭建.md（cmake --preset stm32）"
