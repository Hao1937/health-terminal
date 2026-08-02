# web/ —— Web 看板

`dashboard.html` 是基于 Web Bluetooth 的单页看板。它连接 HM-10/JDY-23 或本仓库的 ESP-WROOM-32 BLE 网关后，按冻结的数据帧格式
接收并解析体测记录，展示身高、体重、BMI、体脂率、心率、血氧、平衡晃动指数、握力、反应时间和
综合评分。页面内置“演示”按钮，脱离硬件也能先验证完整的构帧、分包重组、CRC 校验和数据显示路径。

## 使用步骤

1. 用安卓 Chrome 在安全上下文打开 `dashboard.html`（Web Bluetooth 不支持普通不安全网页；本地调试
   可使用 `localhost` 或 HTTPS 静态服务器）。
2. 先点击“演示”，确认页面能显示假数据；这一步不需要开发板或蓝牙模块。
3. 使用本仓库 ESP32 网关时，按 [ESP32 网关说明](../esp32/ble_gateway/README.md) 接线：STM32 PA2 接 ESP32 RX2（GPIO16），两块板共地；STM32 串口为 **9600 8N1**。
4. 点击“连接 BLE 设备”。页面使用 `acceptAllDevices: true`，设备选择器可能显示附近其他 BLE 设备；
   使用本仓库网关时必须手动选择 `HealthTerminal-Gateway`。连接后页面仍严格获取 `FFE0` 服务并订阅 `FFE1` 特征通知。
5. 在网页显示已连接后，再在终端完成一次测量并进入结果页。固件发送一条 59 字节 RECORD 帧，网页会自动处理 BLE 分包、
   重同步和双层 CRC，然后更新全部字段。

## 协议与故障排查

- 记录 payload 固定 52 字节，帧布局为 `[AA][55][TYPE][LEN lo][LEN hi][PAYLOAD][CRC lo][CRC hi]`，
  RECORD 帧总长 59 字节；多字节字段均为小端。
- 若无法发现设备，检查蓝牙权限、安卓 Chrome 版本、模块供电和 `FFE0/FFE1` 服务/特征 UUID。
- 若能连接但无数据，检查 PA2/PA3 是否交叉连接、是否共地、模块是否处于透传模式以及串口是否为
  9600；网页日志会提示帧 CRC、记录版本或连接错误。
