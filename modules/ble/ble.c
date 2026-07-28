/**
 * @file    ble.c
 * @owner   刘晏铭
 *
 * BLE 只负责把已经收尾的 measurement_record_t 编码为冻结数据帧并透传。
 * 模块配置由 HM-10/JDY-23 的出厂设置负责：USART2 为 9600 8N1，模块上电后
 * 直接进入透传模式。当前板级契约没有复位/状态脚和 UART 接收接口，因此这里
 * 不发送依赖具体固件的 AT 指令，也不实现反向命令通道。
 */
#include "ble.h"

#include "record_codec.h"

#if defined(MODULE_ENABLED_BLE)

#include "uart.h" /* ble_uart_send，仅固件构建可见 */

static int s_ble_ready;

hs_status_t ble_init(void) {
  /* app 已先调用 uart_init()；BLE 模块须已上电并处于透传模式。 */
  s_ble_ready = 1;
  return HS_OK;
}

hs_status_t ble_send_record(const measurement_record_t *rec) {
  if (!s_ble_ready || rec == NULL) return HS_NOT_READY;

  /* BLE 自己收尾副本，避免调用方漏算记录 CRC 或被发送过程修改。 */
  measurement_record_t finalized = *rec;
  record_finalize(&finalized);

  uint8_t frame[FRAME_OVERHEAD + sizeof(measurement_record_t)];
  size_t n = frame_encode_record(&finalized, frame, sizeof(frame));
  if (n == 0) return HS_NOT_READY;
  ble_uart_send(frame, n);
  return HS_OK;
}

#else /* 未启用：桩替代 */

hs_status_t ble_init(void) { return HS_NOT_IMPLEMENTED; }
hs_status_t ble_send_record(const measurement_record_t *rec) {
  (void)rec;
  return HS_NOT_IMPLEMENTED;
}

#endif
