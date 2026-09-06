# BLE 透传与 Web Bluetooth 看板联调

## 1. 链路与职责

```text
STM32 measurement_record_t
→ record_finalize()：记录 CRC
→ frame_encode_record()：59 字节 RECORD 帧
→ USART2 PA2，9600 8N1
→ ESP32 GPIO16/RX2
→ FFE1 Notify（通常 20 + 20 + 19）
→ Android 浏览器 Web Bluetooth
→ 分包重组 + 外层 CRC + 记录 CRC
→ 网页显示
```

职责边界：

- STM32：生成记录、计算 CRC、编码完整二进制帧；
- ESP32：不解析、不改字节、不重算 CRC，只做 UART→BLE 分包转发；
- 网页：重组任意 BLE 分包、校验两层 CRC、按小端解析和显示。

## 2. STM32 USART2

```text
USART2 TX = PA2
USART2 RX = PA3
9600 baud
8 data bits
no parity
1 stop bit
no flow control
```

调试 `printf` 使用 USART1 PA9/PA10、115200 8N1，不得把调试文本混入 PA2 的二进制链路。

`ble_init()` 只表示 STM32 侧 USART2 透传通道准备完成，并不检测：

- ESP32 是否上电；
- 手机是否连接；
- FFE1 是否订阅；
- 数据是否到达网页。

## 3. 冻结帧格式

记录 payload 固定 52 字节，多字节字段均为小端：

```text
AA 55 01 34 00 <52-byte measurement_record_t> <CRC lo> <CRC hi>
```

总长：

```text
5 + 52 + 2 = 59 字节
```

### 3.1 关键字段

| payload 偏移 | 字段 | 单位 |
| ---: | --- | --- |
| 0 | version | 当前 `0x0001` |
| 4 | timestamp | 上电秒数 |
| 8 | height_mm | mm |
| 12 | weight_g | g |
| 16 | bmi_x100 | BMI×100 |
| 20 | bodyfat_x10 | %×10 |
| 24 | heart_rate_bpm | bpm |
| 28 | spo2_x10 | %×10 |
| 32 | balance_x10 | 指数×10 |
| 36 | grip_kg_x10 | kg×10 |
| 40 | reaction_ms | ms |
| 44 | score | 0～100 |
| 48 | record crc16 | 覆盖 payload 前 48 字节 |

无效值 `0x7FFFFFFF` 在网页显示为 `--`。

### 3.2 CRC

```text
CRC16-CCITT
poly = 0x1021
init = 0xFFFF
xorout = 0
```

- 记录 CRC：覆盖 record 前 48 字节；
- 外层帧 CRC：覆盖 `TYPE + LEN + PAYLOAD`，不包含 `AA 55`；
- 两个 CRC 都以小端顺序放入帧中。

## 4. ESP32 DEVKIT V1 网关

程序：

```text
esp32/ble_gateway/ble_gateway.ino
```

### 4.1 最小接线

```text
STM32 PA2 / USART2_TX → ESP32 GPIO16 / RX2
STM32 GND             → ESP32 GND
```

双向命令回传：

```text
STM32 PA3 / USART2_RX ← ESP32 GPIO17 / TX2
```

GPIO17→PA3 是网页反向命令通道，使用“测试双向通信、读取当前记录、同步设备历史”时必须连接。ESP32 建议用自己的 USB 独立供电；不要把 PA2 接到 GPIO3/RX0。

### 4.2 Arduino IDE 烧录

1. 使用 Arduino IDE 2.x；
2. 开发板管理器 URL：

   ```text
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```

3. 安装 Espressif Systems 的 `esp32` Core，建议 3.x；
4. 选择：

   ```text
   Board: ESP32 Dev Module
   Upload Speed: 115200
   Partition Scheme: Default
   Port: 开发板 USB-UART 芯片对应的 COM 口
   ```

   本次 DEVKIT V1 载板通常使用 CP210x；应先在设备管理器识别实际 USB-UART 芯片。缺少驱动时从芯片厂商官方网站获取，例如 Silicon Labs 的 CP210x VCP Driver，不依赖仓库外的第三方资料 ZIP。

5. 使用 Arduino-ESP32 自带 BLE 库，不额外安装同名第三方库；
6. 上传卡在 `Connecting...` 时按住 `BOOT`，必要时按一下 `EN`；出现写入后松开 `BOOT`；
7. 上传结束后按一下 `EN`。

### 4.3 BLE 标识

```text
设备名：HealthTerminal-Gateway

Primary Service：
0000FFE0-0000-1000-8000-00805F9B34FB

Notify Characteristic：
0000FFE1-0000-1000-8000-00805F9B34FB

CCCD：
00002902-0000-1000-8000-00805F9B34FB
```

系统“设置→蓝牙”能看到但不能作为耳机等设备连接是正常现象；这是自定义 BLE GATT 设备，应由网页或 nRF Connect 连接，不要提前在系统设置里配对。

### 4.4 转发策略

- UART FIFO：512 字节；
- Notify 块：最多 20 字节；
- UART 空闲约 5ms 后发送不足 20 字节的尾包；
- notification 间隔约 5ms；
- 断开约 500ms 后恢复广播；
- 一帧通常为 `20 + 20 + 19`。

未连接、未订阅或刚切换订阅状态时，ESP32 会丢弃 UART 数据并清空缓存；FIFO 溢出后会丢弃到 UART 空闲。因此它不是离线缓存器。

## 5. 启动 HTTPS 网页

Web Bluetooth 需要安全上下文。手机直接打开电脑局域网的普通 HTTP 地址通常不可用。

### 5.1 终端一：静态服务器

在仓库根目录运行：

```powershell
python -m http.server 8000 --directory web
```

电脑本地检查：

```text
http://127.0.0.1:8000/dashboard.html
```

### 5.2 终端二：Cloudflare Quick Tunnel

```powershell
cloudflared tunnel --protocol http2 --url http://127.0.0.1:8000
```

如果 `cloudflared` 不在 PATH，可使用安装路径：

```powershell
& "C:\Program Files (x86)\cloudflared\cloudflared.exe" tunnel --protocol http2 --url http://127.0.0.1:8000
```

终端会给出临时网址：

```text
https://随机名称.trycloudflare.com
```

手机打开：

```text
https://随机名称.trycloudflare.com/dashboard.html
```

两个终端必须保持运行。每次重启 Quick Tunnel 通常会生成新网址；旧地址出现 `Error 1033` 时，应重新启动隧道并使用本次新地址。

## 6. Web Bluetooth 页面实现

页面：`web/dashboard.html`。

### 6.1 设备选择兼容

当前使用：

```js
device = await navigator.bluetooth.requestDevice({
  acceptAllDevices: true,
  optionalServices: [BLE_SERVICE]
});
```

原因：部分安卓浏览器按 `FFE0` 广播服务过滤时无法显示 ESP32，即使系统设置已经能看到设备。改为显示所有附近 BLE 设备后，用户手动选择 `HealthTerminal-Gateway`；连接后仍必须成功取得 FFE0/FFE1，因此协议没有放宽。

### 6.2 数据接收

notification 必须按 DataView 实际范围截取：

```js
const value = e.target.value;
new Uint8Array(value.buffer, value.byteOffset, value.byteLength);
```

页面把每个 chunk 追加到 `rxBuf`，搜索 `AA 55`，等待完整长度，再验证：

1. TYPE 为 `0x01`；
2. LEN 为 52；
3. 外层 CRC 正确；
4. record version 正确；
5. record CRC 正确。

断开和重新连接都会清空缓存，避免旧半帧与新记录拼接。

“演示（假数据）”也走完整构帧、CRC、分包解析与渲染路径，可用于先验证网页代码，但不能代替硬件链路验收。

## 7. 唯一正确的联调顺序

1. ESP32上电并开始广播；
2. 手机打开 HTTPS dashboard；
3. 点击“演示”，确认页面正常；
4. 点击“连接 BLE 设备”；
5. 手动选择 `HealthTerminal-Gateway`；
6. 等网页明确显示“已连接”（代表 FFE1 Notify 已开启）；
7. 再完成 STM32 测量；
8. 进入 `6.Score/Upload → Start`；
9. 网页接收 59 字节记录并更新卡片。

STM32 每次进入结果页仍主动发送一次。如果先 Upload、后连接网页，这一帧会被 ESP32 丢弃；但记录已写入 Flash，连接后可在网页点击“同步设备历史”重新获取，无需重新测量。若仍停留在结果页，也可点击“读取当前记录”。

## 8. 分层排障

### 8.1 系统蓝牙可见，网页设备列表不可见

- 确认页面是最新版本并使用 `acceptAllDevices: true`；
- 清除页面缓存或给网址加 `?v=2`；
- 允许浏览器“附近设备/蓝牙”权限；
- 关闭 nRF Connect 和其他 BLE客户端；
- 按 ESP32 `EN` 后重新扫描。

### 8.2 设备可选但连接失败

用 nRF Connect确认：

- FFE0 是 Primary Service；
- FFE1 存在；
- FFE1 支持 Notify、Write 和 Write Without Response；
- 有 `0x2902` CCCD。

确认未被另一个应用连接。

### 8.3 已连接但无数据

依次检查：

1. 是否网页先显示“已连接”；
2. 是否执行 `6.Score/Upload → Start`；
3. STM32是否使用 `MODULE_SET=liuyanming` 或 `full`；
4. PA2 是否接 GPIO16；
5. 两板是否共地；
6. 是否均为 9600 8N1；
7. 逻辑分析仪是否在 PA2 看到：

   ```text
   AA 55 01 34 00 ... 共 59 字节
   ```

8. ESP32是否稳定供电。

### 8.4 CRC 错误

- 不要发送 ASCII 字符串 `"AA 55"`，必须是真实字节 `0xAA 0x55`；
- 检查 USART2是否混入 AT 应答或调试文本；
- 检查是否丢字节；
- 检查 record 52字节、frame 59字节、小端与 CRC覆盖范围。

### 8.5 Cloudflare `Error 1033`

- 本地先检查 `http://127.0.0.1:8000/dashboard.html`；
- 确认 Python终端仍运行；
- 重启 `cloudflared`；
- 使用新输出的 `trycloudflare.com` 地址；
- 不要继续使用失效的旧随机域名。

## 9. 解析器边界保护

网页对 RECORD 帧严格要求 `LEN == 52`，对其他异常长度（包括过大的噪声长度）立即重新寻找帧头；
冻结协议仍为 RECORD 总长 59 字节。若现场持续出现 CRC 错误，应优先排查 UART 串入调试文本、丢字节、供电和共地。
