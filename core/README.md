# core/ —— 芯片底层
STM32F1 的 HAL 库（`STM32CubeF1`，不入库，由 `scripts/fetch_hal.sh` 按需拉取到本目录）、
GCC 启动文件、系统时钟配置，以及链接脚本。链接脚本将 Flash 末 4 页划为独立 EEPROM 区、
与代码区物理隔离，供历史记录存储使用（详见 docs/引脚分配表.md 第 2.9 节）。
