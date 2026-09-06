# BLE + Web 看板联调说明

本文用于验证“STM32 体测结果 ↔ UART/BLE 透传 ↔ Android Chrome 看板”的双向链路。
本仓库支持两种实际接入方式：通用 HM-10/JDY-23 透传模块，或仓库内的
`HealthTerminal-Gateway` ESP32 网关（STM32 PA2 → ESP32 GPIO16/RX2）。BLE 模块负责
UART 到无线 GATT 的透传；STM32 不实现 BLE 协议栈，也不接收网页命令。

## 硬件与接线

### 通用 HM-10/JDY-23 透传模块

| STM32 | BLE 模块 | 说明 |
| --- | --- | --- |
| PA2 / USART2_TX | RX | 交叉连接 |
| PA3 / USART2_RX | TX | 交叉连接 |
| GND | GND | 必须共地 |
| 3.3 V 或模块规定电源 | VCC | 按实际模块/载板规格供电 |

串口固定为 **9600 8N1、无硬件流控**。不要把未经确认的 5V UART 输出直接接入 STM32 的 PA2/PA3。

### 本仓库 ESP32 网关

双向接线为 `STM32 PA2/USART2_TX → ESP32 GPIO16/RX2`、`STM32 PA3/USART2_RX ← ESP32 GPIO17/TX2`，并连接 `GND → GND`；ESP32
建议独立 USB 供电。网关广播 `HealthTerminal-Gateway`，提供 FFE0 Primary Service
和 FFE1 Notify/Write 特征，详细烧录步骤见
[esp32/ble_gateway/README.md](../esp32/ble_gateway/README.md)。
当前板级契约没有 BLE `STATE`、`RESET` 或 `KEY` 引脚，因此 `ble_init()` 返回 `HS_OK` 只表示
MCU 侧已启用 USART2 透传通道，不表示手机已连接。HM-10/JDY-23 应预先配置为默认 9600 波特率并
上电进入透传模式；本项目不会在透传数据中盲发型号相关的 AT 指令。

## 网页验证

1. 用 Android Chrome 在 HTTPS 或 `localhost` 安全上下文打开 `web/dashboard.html`。
2. 点击“演示”，确认身高、体重、BMI、体脂率、心率、血氧、平衡、握力、反应时间和综合评分均显示。
3. 确认页面日志没有“记录 CRC 失败”或“帧 CRC/类型不符”。
4. 点击“连接 BLE 设备”。网页使用 `acceptAllDevices: true`，设备选择器可能显示附近其他
   BLE 设备；本仓库网关必须手动选择 `HealthTerminal-Gateway`。连接后网页仍会严格获取
   `FFE0` 服务和 `FFE1` 通知特征。如果通用模块 UUID 不同，应先用 BLE 扫描工具确认，再调整网页配置，
   不要修改 STM32 帧协议。
5. 在终端完成一次测量并进入结果页；也可由网页请求当前记录或同步 Flash 历史。
6. 逐项比较 OLED/调试串口中的记录值与网页卡片值。页面会自动合并 BLE 分包，校验外层帧 CRC 和
   记录内部 CRC，并将定点整数还原为显示单位。
7. 断开后重新连接，再上传一条记录，确认不会把上次连接残留的半帧与新记录拼接。

## 帧格式

记录 payload 固定 52 字节，RECORD 帧总长 59 字节：

```text
AA 55 01 34 00 <52-byte measurement_record_t> <CRC lo> <CRC hi>
```

所有多字节字段均为小端。外层 CRC16-CCITT 覆盖 `TYPE + LEN + PAYLOAD`，记录自身 CRC 覆盖前
48 字节。编码和校验统一复用 `algorithms/record_codec.c`，网页不假设一次 Bluetooth notification
就是一整帧。

## 常见问题

- **网页不支持 Bluetooth**：使用 Android Chrome，并通过 HTTPS/`localhost` 访问；普通不安全地址通常
  不具备 Web Bluetooth 权限。
- **扫描不到模块**：检查供电、蓝牙权限和模块是否广播；确认实际服务 UUID，不要只依据模块名称判断。
- **已连接但网页无数据**：检查 TX/RX 是否交叉、是否共地、串口是否为 9600、模块是否仍在 AT 模式，
  以及 `FFE1` 是否开启 Notify，并具备 Write/Write Without Response。
- **日志提示 CRC 错误**：检查是否有其他设备向 USART2 发送调试文本；确认固件和网页都使用冻结 52/59
  字节协议，且没有把 AT 应答混入透传数据。

## 构建验证

```bash
cmake --preset host
cmake --build --preset host
ctest --preset host --output-on-failure

cmake --preset stm32 -DMODULE_SET=liuyanming
cmake --build --preset stm32

cmake --preset stm32 -DMODULE_SET=full
cmake --build --preset stm32
```

真机验收建议保存：模块准确型号和固件版本、服务/特征 UUID、手机 Chrome 版本、网页演示截图、
一次 59 字节原始帧，以及各体测字段的终端值与网页值对照表。
