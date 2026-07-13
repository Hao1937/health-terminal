# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

基于 STM32F103C8T6 的健康体测终端：一次完成 8 项测量（身高/体重/BMI·体脂/心率/血氧/平衡/握力/反应时间）
并给出综合评分。裸机主循环状态机（无 RTOS），OLED + 矩阵键盘操作，片内 Flash 存历史，BLE 透传到
Web Bluetooth 网页看板。3 人异地并行开发（王宇浩[组长] / 查樊听 / 刘晏铭）。

## 常用命令

```bash
# 一键就绪（获取 HAL + 检查工具链 + 跑 host 测试）
./scripts/bootstrap.sh          # Windows: .\scripts\bootstrap.ps1

# 单独获取 HAL（STM32CubeF1，不入库；仅 sparse-clone Drivers，约 27MB）
bash scripts/fetch_hal.sh

# host 单元测试（纯 PC，无需开发板）——每次合并前的最低门槛
cmake --preset host && cmake --build --preset host && ctest --preset host --output-on-failure

# 只跑单个测试
ctest --preset host -R test_filters          # test_filters | test_codec | test_algorithms

# 交叉编译固件（产物 build/stm32/firmware.{elf,bin,hex}）
cmake --preset stm32 && cmake --build --preset stm32
cmake --preset stm32 -DMODULE_SET=yuhao      # 只把某人的模块编成真实现，其余为桩

# 烧录
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
        -c "program build/stm32/firmware.elf verify reset exit"
```

固件版本变更后需重新 `cmake --preset stm32` 才会重跑 firmware.cmake 的配置逻辑。

## 构建系统的两个关键机制

**1. 单一 CMakeLists，两种构建目标（`HT_BUILD`）。** 由 preset 选择：
- `host` preset → `HT_BUILD=host`：只编 `algorithms/`（静态库 `health_algo`）+ `tests/`，走系统编译器。
- `stm32` preset → `HT_BUILD=firmware`：`include` 进 `cmake/firmware.cmake`，用 `cmake/arm-none-eabi.cmake`
  工具链交叉编译。`algorithms/` 是纯 C、无硬件依赖，两种构建共用同一份源码。

**2. MODULE_SET 用宏切换「真实现/桩」，而非增删源文件（非直觉，改动模块时务必理解）。**
`modules/<mod>/<mod>.c` **永远都会被编译**；每个文件内部用 `#if defined(MODULE_ENABLED_<NAME>)`
在「真实现」与「返回 `HS_NOT_IMPLEMENTED` 的桩」两个分支间切换。`firmware.cmake` 按 `MODULE_SET`
（`yuhao`/`chafanting`/`liuyanming`/`full`）只对启用模块 `-DMODULE_ENABLED_<NAME>=1`。
好处：无论编哪个子集都不产生符号重复、`app` 恒可链接。因此**新写模块时，真实现和桩这两个分支都要给全**
（见 `modules/max30102/max30102.c` 的样板）。

## 架构：契约优先 + 注册表驱动

**冻结的公共契约在 `include/`**（改动需三人评审 + 同步 `web/dashboard.html` 解析）：
- `health_if.h`：状态码 `hs_status_t`、统一传感器签名 `xxx_init()/xxx_measure()`、`hs_sample_t`、
  测量项枚举 `hs_item_t`、注册表类型 `hs_sensor_t`。
- `health_record.h`：`measurement_record_t`（**固定 52 字节**，含 `sizeof==52` 编译期断言）、
  UART/BLE 帧格式、CRC16-CCITT 常量。所有物理量用**定点整数**（如 `bmi_x100`），全程**小端**。

**数据流：** `app` 主状态机（`ST_BOOT→MENU→MEASURE→RESULT→HISTORY`，见 `app/app.h`）遍历
`app/app_sensors.c` 的 `g_hs_registry[]`（把 `hs_item_t` 映射到各模块函数指针，哨兵 `measure==NULL` 结尾）
逐项采集 → 结果填入 `g_current_record` → `algorithms/` 计算派生量（BMI/体脂不占传感器表项，由 app 算）
与综合评分 → `record_codec.c` 打包成帧 → 经 `bsp/` 串口/BLE 发出、或存入片内 Flash。
新增传感器只需实现两个函数并在注册表加一行，状态机代码不动。

**分层：** `bsp/`（封装 HAL，屏蔽寄存器）← `modules/`（传感器驱动）+ `algorithms/`（纯算法）← `app/`（编排）。
上层只依赖下层导出的接口。

## HAL 与底层（`core/`）

- `STM32CubeF1` 不入库，由 `fetch_hal.sh` 拉取；`firmware.cmake` 找不到会直接 FATAL 报错并提示获取命令。
- `firmware.cmake` 只挑选用到的 HAL 源文件编译（控制体积，避免撑爆 64KB Flash）。
- 启动文件与 `system_stm32f1xx.c` 用 CubeF1 官方模板；**链接脚本 `core/ldscripts/STM32F103C8Tx_FLASH.ld`
  用工程自带版本**，把 Flash 末 4 页（4KB）划为独立 EEPROM 段供 `storage` 历史存储，与代码区物理隔离。
- 引脚分配是冻结契约，与 `core/include/board.h` 宏一一对应，详见 `docs/引脚分配表.md`（改动前须全员确认）。

## 协作与 CI

- 分支：`main`（受保护，只放可演示版）← `dev`（集成主线，每周日合并，合并后 host 须全绿）←
  `feature/姓名-模块`。详见 `docs/协作流程.md`。
- CI（`.github/workflows/ci.yml`）：每次 push/PR 跑 host 单元测试 + 用 4 种 `MODULE_SET` 矩阵交叉编译；
  `full` 固件作为产物上传。

## 编码约定（Windows 多人协作易踩坑）

- 源码含 UTF-8 中文注释：`CMakeLists.txt` 对 MSVC 加 `/utf-8`，否则报 C4819。
- `.gitattributes` 固定行尾：`*.sh` 与 `*.ld` 为 LF、`*.ps1` 为 CRLF。**`.sh` 不可加 BOM**（会破坏 shebang）。
- `bootstrap.ps1` 存为 **UTF-8 with BOM**，否则 Windows PowerShell 5.1 按 GBK 读取导致中文乱码。
