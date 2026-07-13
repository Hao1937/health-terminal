# health-terminal · 多维度智能健康体测终端

基于 STM32F103C8T6 的多维度智能健康体测终端：一次完成 **身高、体重、BMI/体脂率、
心率、血氧、平衡、握力、反应时间** 8 项测量，并给出**综合健康评分**，支持 OLED 菜单、
矩阵键盘操作、片内 Flash 历史记录、BLE 透传与 Web Bluetooth 网页看板。

> 嵌入式系统高级实验 · 3 人团队（王宇浩[组长]、查樊听、刘晏铭）· 裸机 + 主循环状态机（无 RTOS）

## 里程碑时间线
| 时间 | 节点 | 说明 |
|---|---|---|
| 7.8 | 开题 | 确定方案、冻结数据格式与引脚分配 |
| 7 月 | 分散开发 | 异地各自开发板 + 各自传感器，逐个把模块桩替换为真实现 |
| **8.10** | **中期检查** | 交 PPT，需各模块**单体实测数据**；打 tag `v0.5-midterm` |
| 9 月初 | 集中集成 | 一周整机联调（MODULE_SET=full） |
| **9.8** | **最终验收** | 8 项测量 + 评分 + 看板 + 历史全打通；打 tag `v1.0-final` |

## 三步快速上手
```bash
# 1) 克隆仓库
git clone <repo-url> health-terminal && cd health-terminal

# 2) 一键就绪：获取 HAL(只拉 Drivers) + 检查工具链 + 跑 host 单元测试
./scripts/bootstrap.sh        # Windows 用 PowerShell 跑 ./scripts/bootstrap.ps1

# 3) 交叉编译并烧录首版固件（LED 闪烁 + 串口打印版本号）
cmake --preset stm32 && cmake --build --preset stm32
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
        -c "program build/stm32/firmware.elf verify reset exit"
```
只想跑算法单元测试（不需要开发板）：
```bash
cmake --preset host && cmake --build --preset host && ctest --preset host
```
每人只编译自己的模块子集：`cmake --preset stm32 -DMODULE_SET=yuhao|chafanting|liuyanming|full`

## 目录导览
```
health-terminal/
├── include/        统一契约头：health_if.h（接口+状态码）、health_record.h（数据/帧格式）
├── core/           芯片底层：STM32CubeF1(HAL, scripts/fetch_hal.sh 按需获取)、启动文件、时钟、链接脚本(隔离EEPROM区)
├── bsp/            板级支持：i2c_bus / uart / gpio / delay / flash_eeprom
├── modules/        外设驱动：每个传感器一个子目录，统一 init/measure 接口
├── algorithms/     纯算法（PC 可编译测试）：filters / spo2_hr / balance / bodyfat_navy / health_score / crc16 / record_codec
├── app/            裸机主状态机、UI 流程、传感器注册表、main.c
├── web/            dashboard.html —— Web Bluetooth 单页看板（安卓 Chrome）
├── tests/          host 单元测试（ctest）
├── docs/           接口文档 / 引脚分配表 / 开发环境搭建 / 协作流程
├── cmake/          交叉编译工具链文件、固件构建脚本
├── scripts/        bootstrap 一键脚本
└── CMakePresets.json   两个 preset：host（PC 测试）/ stm32（交叉编译固件）
```

## 分工一览
| 成员 | 负责范围 |
|---|---|
| 王宇浩（组长） | bsp 底层、滤波公共库、MAX30102 心率血氧、握力(HX711#2)、综合评分、整机集成 |
| 查樊听 | 体重(HX711#1)与标定、身高(HC-SR04)+温度补偿(DS18B20) |
| 刘晏铭 | OLED 菜单与趋势、矩阵键盘、反应时间、片内 Flash 历史、BLE+Web 看板、MPU6050 平衡 |

## 持续集成（CI）
仓库配了 GitHub Actions（`.github/workflows/ci.yml`）：每次 `push` 或 PR 会自动
**跑 host 单元测试 + 用 4 种 `MODULE_SET` 交叉编译固件**，绿勾才建议合并（对齐
[协作流程](docs/协作流程.md)「合并前 host 全绿」）。`full` 集成版固件会作为构建产物上传。

## 文档索引
- [接口文档](docs/接口文档.md) · [引脚分配表](docs/引脚分配表.md) · [开发环境搭建](docs/开发环境搭建.md) · [协作流程](docs/协作流程.md)
- 第一周分头开工清单见下方 [docs/第一周行动清单.md](docs/第一周行动清单.md)
