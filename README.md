# health-terminal · 多维度智能健康体测终端

基于 **STM32F103C8T6** 的多维度智能健康体测终端：一次完成 **身高、体重、BMI/体脂率、心率、
血氧、平衡、握力、反应时间** 8 项测量，并给出**综合健康评分**。支持 OLED 菜单、矩阵键盘操作、
片内 Flash 历史记录、BLE 透传与 Web Bluetooth 网页看板。

> 嵌入式系统高级实验 · 3 人团队（王宇浩[组长]、查樊听、刘晏铭）· 裸机主循环状态机（无 RTOS）

## 里程碑时间线

| 时间 | 节点 | 交付要求 |
|---|---|---|
| 7.8 | 开题 | 确定方案，冻结数据格式与引脚分配 |
| 7 月 | 分散开发 | 异地各自开发板 + 传感器，逐个把模块桩替换为真实现 |
| **8.10** | **中期检查** | 交 PPT，各模块需**单体实测数据**；打 tag `v0.5-midterm` |
| 9 月初 | 集中集成 | 一周整机联调（MODULE_SET=full） |
| **9.8** | **最终验收** | 8 项测量 + 评分 + 看板 + 历史全打通；打 tag `v1.0-final` |

## 三步快速上手

```bash
# 1) 克隆仓库
git clone <repo-url> health-terminal && cd health-terminal

# 2) 一键就绪：获取 HAL + 检查工具链 + 跑 host 单元测试
./scripts/bootstrap.sh        # Windows 用 ./scripts/bootstrap.ps1

# 3) 交叉编译并烧录首版固件（LED 闪烁 + 串口打印版本号）
cmake --preset stm32 && cmake --build --preset stm32
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
        -c "program build/stm32/firmware.elf verify reset exit"
```

只跑算法单元测试（无需开发板）：

```bash
cmake --preset host && cmake --build --preset host && ctest --preset host
```

每人只编译自己的模块子集：`cmake --preset stm32 -DMODULE_SET=yuhao|chafanting|liuyanming|full`

## 目录导览

```
health-terminal/
├── include/        公共契约头：health_if.h（接口+状态码）、health_record.h（数据/帧格式）
├── core/           芯片底层：STM32CubeF1(HAL, 按需获取)、启动文件、时钟、链接脚本（隔离 EEPROM 区）
├── bsp/            板级支持：i2c_bus / uart / gpio / delay / flash_eeprom
├── modules/        外设驱动：每个传感器一个子目录，统一 init/measure 接口
├── algorithms/     纯算法（PC 可编译测试）：filters / spo2_hr / balance / bodyfat / health_score / crc16 / record_codec
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

## 新成员上手全流程（示例）

下面以**查樊听第一次上手、负责体重模块 `hx711_weight`** 为例，把从零到提交的完整流程走一遍。
其他成员把"模块名/引脚/负责人"换成自己的即可。**每一步都标注了要读哪份文档、要执行什么命令。**

### 第 0 步：先读三份文档（约半小时，别跳过）

按顺序通读，建立整体认知：

1. 本 README —— 了解项目做什么、目录长什么样、自己负责哪块。
2. [docs/开发环境搭建.md](docs/开发环境搭建.md) —— 待会儿照它装工具、编译、烧录。
3. [docs/协作流程.md](docs/协作流程.md) —— 了解分支模型与提交规范，避免一上来就把 `main` 弄乱。

接口和引脚两份文档**先扫一眼有印象**即可，写代码时（第 4 步）再精读对应章节。

### 第 1 步：装环境 + 克隆 + 跑通 host 自检

照 [docs/开发环境搭建.md](docs/开发环境搭建.md) 第 1~3 节装齐工具链（Git、CMake ≥3.21、Ninja、
arm-none-eabi-gcc、OpenOCD、host 编译器），然后：

```bash
git clone <repo-url> health-terminal && cd health-terminal
./scripts/bootstrap.sh        # Windows: .\scripts\bootstrap.ps1
```

`bootstrap` 会自动获取 HAL、检查工具、跑一遍 host 单元测试。看到
`test_filters / test_codec / test_algorithms` 全部 **Passed**，就说明环境 OK。
（若中途报缺工具，回到开发环境搭建对应小节补装。）

### 第 2 步：烧一版 hello 固件，确认板子和烧录链路正常

参见 [docs/开发环境搭建.md](docs/开发环境搭建.md) 第 7~8 节，接好 ST-Link（SWD 4 线）与
USB-TTL 串口（看日志），然后：

```bash
cmake --preset stm32 && cmake --build --preset stm32
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
        -c "program build/stm32/firmware.elf verify reset exit"
```

预期现象：**PC13 板载 LED 闪烁** + 串口（115200 8N1）打印版本号。看到这两个，说明硬件、工具链、
烧录三条链路都通了——此后出问题就能排除"环境本身有病"。

### 第 3 步：从 dev 切出自己的分支

参见 [docs/协作流程.md](docs/协作流程.md) 第 1 节。**永远不要直接在 `main`/`dev` 上写代码。**

```bash
git checkout dev && git pull origin dev
git checkout -b feature/chafanting-hx711-weight     # feature/姓名-模块
```

### 第 4 步：精读接口契约，理解自己模块要实现成什么样

现在精读 [docs/接口文档.md](docs/接口文档.md)，重点三处：

- **第 2 节 状态码** —— 明白 `measure` 什么时候返回 `HS_OK` / `HS_UNSTABLE` / `HS_TIMEOUT`。
- **第 3 节 传感器接口** —— 自己的模块必须实现 `hx711_weight_init()` 与 `hx711_weight_measure()`，
  结果写进 `out->primary`（体重，单位见 record 字段：`weight_g` 克）。
- **第 4 节 record 字段** —— 确认自己负责的字段是 `weight_g`、单位是克（定点整数）。

再查 [docs/引脚分配表.md](docs/引脚分配表.md) 找到本模块的引脚（HX711#1：SCK=PA1、DOUT=PB11），
接线一律以此表为准。

### 第 5 步：写代码——先桩，再真实现（两个分支都要给全）

打开 `modules/hx711_weight/hx711_weight.c`。注意本项目的关键约定：**文件里用
`#if defined(MODULE_ENABLED_HX711_WEIGHT)` 分成「真实现」和「桩」两个分支，两个都要保留**
（原理见 [CLAUDE.md](CLAUDE.md)「构建系统的两个关键机制」，参考样板 `modules/max30102/max30102.c`）：

```c
#if defined(MODULE_ENABLED_HX711_WEIGHT)
hs_status_t hx711_weight_measure(hs_sample_t *out)
{
    /* 读 24bit 原始值 → 减皮重 → 乘标定系数得克数 */
    if (/* 数据未就绪 */ 0) return HS_NOT_READY;
    if (/* 读数不稳   */ 0) return HS_UNSTABLE;
    out->primary   = /* weight_g */ 0;
    out->secondary = HS_VALUE_INVALID;
    return HS_OK;
}
#else   /* 未启用本模块时的桩，保证 app 恒可链接 */
hs_status_t hx711_weight_measure(hs_sample_t *out) { (void)out; return HS_NOT_IMPLEMENTED; }
#endif
```

模块已在 `app/app_sensors.c` 的注册表里登记过，函数签名不变就无需改动 app 层。

### 第 6 步：测试自己的模块

分两层，缺一不可：

**(a) host 单元测试（纯算法，不用板子）。** 如果你的模块含可在 PC 上验证的算法（如标定换算），
在 `tests/` 加 `test_*.c` 并在 `tests/CMakeLists.txt` 的 `HT_TESTS` 里登记，然后：

```bash
cmake --preset host && cmake --build --preset host
ctest --preset host --output-on-failure      # 只跑自己的：ctest --preset host -R test_xxx
```

**(b) 单模块上板实测（驱动时序、真实读数）。** 只编自己的模块子集烧板，省时且互不干扰：

```bash
cmake --preset stm32 -DMODULE_SET=chafanting     # 只把查樊听的模块编成真实现
cmake --build --preset stm32
# 烧录同第 2 步
```

上板后用标准砝码（如 5/10/20kg）实测并记录读数与误差——**这份单体实测数据是 8.10 中期检查的硬性要求**。

### 第 7 步：提交

参见 [docs/协作流程.md](docs/协作流程.md) 第 3 节，用 Conventional Commits 格式，一次提交只做一件事：

```bash
git add modules/hx711_weight/
git commit -m "feat(hx711_weight): 实现体重读数与两点标定，实测 10kg 砝码误差<20g"
```

### 第 8 步：合并前自检 + 推分支 + 合并到 dev

**合并前务必对照 [docs/协作流程.md](docs/协作流程.md) 第 2 节的检查清单**（本地能编译、
`ctest --preset host` 全绿、实测数据已记录、没混入 build 产物、没擅改公共契约）。然后：

```bash
git fetch origin && git rebase origin/dev     # 先把分支追到最新 dev
ctest --preset host                            # 再确认一次全绿
git push origin feature/chafanting-hx711-weight
```

在 GitHub 上发起指向 `dev` 的 PR。**CI 会自动跑 host 测试 + 4 种 MODULE_SET 交叉编译**，
等绿勾出现再合并（改了 `include/` 等公共契约还需另一名成员 review）。周日合并日由组长择稳定点合入 `main`。

> 一句话记住流程：**读文档 → 装环境跑通自检 → 切分支 → 照接口写(桩+真实现) → host测+上板实测 → 规范提交 → 自检全绿 → 推分支发 PR 等 CI 绿。**

## 持续集成（CI）

仓库配置了 GitHub Actions（`.github/workflows/ci.yml`）：每次 `push` 或 PR 会自动
**跑 host 单元测试 + 用 4 种 `MODULE_SET` 交叉编译固件**，全绿方建议合并。`full` 集成版固件
作为构建产物上传。

## 文档索引

- [接口文档](docs/接口文档.md) · [引脚分配表](docs/引脚分配表.md) · [开发环境搭建](docs/开发环境搭建.md) · [协作流程](docs/协作流程.md)
- 分头开工任务清单：[docs/中期任务清单.md](docs/中期任务清单.md)
