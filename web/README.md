# Web 看板与本地部署

`dashboard.html` 展示统一记录中的身高、体重、BMI、体脂率、心率、血氧、平衡、握力、反应时间和综合评分。所有收到的记录都必须先通过外层帧 CRC 和记录 CRC，随后才会显示并保存到浏览器 `localStorage`。

## 已支持功能

- STM32 主动上传最新综合记录；
- 网页发送 `PING`、`GET CURRENT`、`GET HISTORY`；
- STM32 返回当前缓存或 Flash 中最多 76 条历史记录；
- 浏览器本地保留最近 200 条去重记录，并支持 CSV 导出；
- BLE 意外断开后尝试三次自动重连；
- 对异常长度和 CRC 错误进行帧头重同步。

队友模块不需要单独修改网页协议。身高、体重、心率/血氧和握力均由 `app/app_statemachine.c` 写入同一个 `measurement_record_t`，网页按固定偏移解析。

## 双向接线

```text
STM32 PA2 / USART2_TX → ESP32 GPIO16 / RX2
STM32 PA3 / USART2_RX ← ESP32 GPIO17 / TX2
STM32 GND             — ESP32 GND
```

两边均为 3.3V UART、9600 8N1。ESP32 建议独立 USB 供电。新版网关必须重新烧录，FFE1 同时具备 Notify、Write 和 Write Without Response 属性。

## 电脑本机部署

在仓库根目录运行：

```powershell
node web/serve_local.mjs
```

然后用本机 Chrome 打开：

```text
http://localhost:8000/dashboard.html
```

`localhost` 属于浏览器认可的安全来源，可以使用 Web Bluetooth。

## 手机在局域网内访问

手机通过 `http://电脑局域网IP:8000` 访问时不属于安全来源，Web Bluetooth 会被禁用，因此必须使用浏览器信任的 HTTPS 证书。

可用 `mkcert` 为电脑局域网 IP 生成开发证书，并让手机信任 mkcert 的本地根证书。例如电脑 IP 为 `192.168.1.20`：

```powershell
mkcert -install
mkcert 192.168.1.20 localhost 127.0.0.1
node web/serve_local.mjs --host 0.0.0.0 --port 8443 --cert .\.local-certs\lan.pem --key .\.local-certs\lan-key.pem
```

手机与电脑接入同一 Wi-Fi，在手机 Chrome 打开：

```text
https://192.168.1.20:8443/dashboard.html
```

证书与私钥必须放在网页目录之外（本项目使用 `.local-certs/`）；Windows 防火墙需要允许 Node.js 监听该端口。把公开的 `rootCA.pem` 单独传到手机并安装为 CA 证书，绝不能传输 `rootCA-key.pem` 或 `lan-key.pem`。若手机没有信任签发该证书的根 CA，即使页面能强行打开，也可能仍不满足 Web Bluetooth 的安全上下文要求。

## 操作顺序

1. 重新烧录 STM32 固件和 `esp32/ble_gateway/ble_gateway.ino`；
2. 确认 PA2、PA3 交叉连接并共地；
3. 打开网页并连接 `HealthTerminal-Gateway`；
4. 点击“测试双向通信”，日志应显示 `PONG health-terminal v0.1`；
5. 点击“读取当前记录”或“同步设备历史”；
6. 完成队友及本人模块的测量，再执行终端的 `Score/Upload`；
7. 对照 OLED、网页当前值和历史表，必要时导出 CSV。

网页不会远程启动传感器，也不会改写 STM32 中的测量值或删除 Flash；这三个限制用于避免手机误操作硬件测试。
