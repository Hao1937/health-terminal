/**
 * @file    ble.c
 * @owner   刘晏铭
 *
 * 发送路径已给出可用参考实现（编码+串口发送），对齐冻结帧格式，供整机联调与
 * web 看板演示。TODO(刘晏铭)：AT
 * 初始化配置（名称/波特率/连接参数）与接收侧命令解析。
 */
#include "ble.h"

#include "record_codec.h"

#if defined(MODULE_ENABLED_BLE)

#include "uart.h" /* ble_uart_send，仅固件构建可见 */

hs_status_t ble_init(void) {
  /* TODO(刘晏铭)：通过 AT 指令配置模块名/波特率；此处假定上电即透传就绪 */
  return HS_OK;
}

hs_status_t ble_send_record(const measurement_record_t *rec) {
  uint8_t frame[FRAME_OVERHEAD + sizeof(measurement_record_t)];
  size_t n = frame_encode_record(rec, frame, sizeof(frame));
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
