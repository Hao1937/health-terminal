/**
 * @file    ble.h
 * @brief   BLE 透传（JDY-23/HM-10，走 USART2），按帧格式发送体测记录。
 * @owner   刘晏铭
 *
 * 发送侧直接复用已冻结的 record_codec 帧格式，web/dashboard.html 据此解析。
 * ble_init() 只确认 MCU 侧透传通道已启用，不代表手机已经连接。
 */
#ifndef MODULE_BLE_H
#define MODULE_BLE_H

#include "health_if.h"
#include "health_record.h"

typedef enum {
  BLE_CMD_NONE = 0,
  BLE_CMD_PING,
  BLE_CMD_GET_CURRENT,
  BLE_CMD_GET_HISTORY,
  BLE_CMD_UNKNOWN,
} ble_command_t;

hs_status_t ble_init(void);
/** @brief 把一条记录打包成数据帧并经 BLE 串口发出。 */
hs_status_t ble_send_record(const measurement_record_t *rec);
/** @brief 发送带 CRC 的文本状态帧（FRAME_TYPE_HELLO）。 */
hs_status_t ble_send_status(const char *text);
/** @brief 轮询网页经 ESP32 转发来的换行结尾 ASCII 命令。 */
ble_command_t ble_poll_command(void);

#endif /* MODULE_BLE_H */
