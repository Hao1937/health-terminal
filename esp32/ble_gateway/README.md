# ESP32 BLE 网关

本目录中的 `ble_gateway.ino` 将 STM32 USART2 的二进制数据透明转发到 BLE：

```text
STM32 PA2 / USART2_TX
        ↓ 9600 8N1
ESP32 RX2(GPIO16)
        ↓ BLE Notify
FFE0 / FFE1 → Android Chrome Web Bluetooth
```

ESP32 不解析健康记录、不重算 CRC、不添加文本。STM32 已经输出完整的 59 字节 RECORD 帧，网关只按顺序分成最多 20 字节的 BLE notification。网页会自动重组分包。

## 1. 准备软件

1. 安装 Arduino IDE 2.x。
2. 打开 Arduino IDE，进入“文件 → 首选项”，在“附加开发板管理器网址”加入：

   ```text
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```

3. 进入“工具 → 开发板 → 开发板管理器”，搜索并安装 `esp32`（Espressif Systems）。
4. 选择开发板：

   ```text
   工具 → 开发板 → esp32 → ESP32 Dev Module
   ```

5. 通过 USB-C 数据线连接开发板，在“工具 → 端口”选择 CP210x 对应的 `COMx`。

如果设备管理器没有出现串口，请先确认 USB-C 线支持数据传输，并识别开发板上的
USB-UART 芯片型号；若为 Silicon Labs CP210x，请从 [Silicon Labs 官方页面](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)
下载对应的 VCP Driver。不要依赖或提交第三方资料包中的驱动安装文件。

## 2. 编译与烧录

1. 在 Arduino IDE 中打开本目录的 `ble_gateway.ino`。
2. 设置：
   - Board：`ESP32 Dev Module`
   - Port：CP210x 对应的 `COMx`
   - Upload Speed：`115200`
   - Partition Scheme：`Default`
3. 点击“验证”确认编译。
4. 点击“上传”。如果一直停在 `Connecting...`：
   - 按住开发板 `BOOT` 键；
   - 再点击上传或按一下 `EN/RESET`；
   - 出现 `Writing at ...` 后松开 `BOOT`。
5. 上传完成后按一下 `EN/RESET`，程序会重新广播。

本工程使用 Arduino-ESP32 自带 BLE 库。如果你的 Core 版本导致 API 编译错误，优先使用 Arduino IDE 2.x 与 Espressif `esp32` Core 3.x；不要额外安装同名第三方 BLE 库。

## 3. ESP32 与 STM32 接线

| STM32F103C8T6 | ESP32-WROOM-32 开发板 | 说明 |
|---|---|---|
| `PA2 / USART2_TX` | `RX2`（GPIO16） | STM32 数据进入 ESP32 |
| `PA3 / USART2_RX` | `TX2`（GPIO17） | 当前固件没有反向 BLE 命令，可选接 |
| `GND` | `GND` | 必须共地 |

当前数据方向最低只需要：

```text
PA2 → RX2
GND → GND
```

两边都是 3.3 V UART。不要把 5 V UART 直接接入 ESP32；不要把 STM32 数据接到 `RX0/TX0`。`RX0/TX0`（GPIO3/GPIO1）是 USB 串口下载和调试接口，`RX2/TX2` 才是本网关的数据接口。ESP32 建议使用 USB 独立供电，不要直接由 STM32 小功率 3.3 V 稳压器供电。

## 4. BLE 连接

网关广播名称为：

```text
HealthTerminal-Gateway
```

GATT 服务固定为：

```text
Primary Service: 0000FFE0-0000-1000-8000-00805F9B34FB
Notify Characteristic: 0000FFE1-0000-1000-8000-00805F9B34FB
Descriptor: 00002902-0000-1000-8000-00805F9B34FB
```

手机使用 Android Chrome 打开 HTTPS 看板，例如：

```text
https://你的隧道地址.trycloudflare.com/dashboard.html
```

然后按以下顺序：

1. 点击网页“演示（假数据）”，确认网页正常；
2. 点击“连接 BLE 设备”；
3. 选择 `HealthTerminal-Gateway`；
4. 等网页显示“已连接”；
5. 再在 STM32 上完成测量并进入结果页。

不要先在手机系统蓝牙设置中配对；Web Bluetooth 是在网页设备选择器中连接。当前 STM32 进入结果页时只发送一次，网页必须先完成通知订阅。

## 5. 调试与故障排查

默认不会从 USB 串口输出调试文本，避免干扰数据链路。临时调试时可在 `.ino` 中将：

```cpp
#define GATEWAY_DEBUG 0
```

改为 `1`，再从 USB 串口监视器查看连接、订阅、断开和 FIFO 溢出信息。调试日志只走 `Serial`，绝不能写入 `Serial2`。

使用 nRF Connect 检查：

- 是否广播 `HealthTerminal-Gateway`；
- 广播中是否包含 `FFE0`；
- `FFE0` 是否为 Primary Service；
- `FFE1` 是否有 Notify；
- `FFE1` 是否存在 `0x2902` CCCD。

若网页能连接但无数据显示：

- 确认先连接网页再重新测量；
- 检查 `PA2 → RX2`、GND 共地；
- 检查 STM32 USART2 是否为 `9600 8N1`；
- 确认未把调试文本接入 `PA2`；
- 用逻辑分析仪观察 PA2 是否出现以 `AA 55 01 34 00` 开头、总长 59 字节的帧。

若网页提示 CRC 错误，不要用串口工具发送 ASCII 文本或十六进制字符，例如字符串 `AA 55`；必须发送真实二进制字节。网关会把 59 字节帧通常分为 `20 + 20 + 19` 三个 notification，网页会自动拼接。
