/**
 * @file    ble.h
 * @brief   BLE 透传（JDY-23/HM-10，走 USART2），按帧格式发送体测记录。
 * @owner   刘晏铭
 *
 * 发送侧直接复用已冻结的 record_codec 帧格式，web/dashboard.html 据此解析。
 */
#ifndef MODULE_BLE_H
#define MODULE_BLE_H

#include "health_if.h"
#include "health_record.h"

hs_status_t ble_init(void);
/** @brief 把一条记录打包成数据帧并经 BLE 串口发出。 */
hs_status_t ble_send_record(const measurement_record_t *rec);

#endif /* MODULE_BLE_H */
