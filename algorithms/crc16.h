/**
 * @file    crc16.h
 * @brief   CRC16-CCITT（poly 0x1021, init 0xFFFF, 不反转）。
 * @owner   王宇浩（组长）
 *
 * record 的完整性校验、UART/BLE 帧校验、web 端解析三处共用同一算法。
 * 纯 C 无硬件依赖，可在 PC 编译并单元测试。
 */
#ifndef CRC16_H
#define CRC16_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 计算 buf[0..len) 的 CRC16-CCITT。 */
uint16_t crc16_ccitt(const uint8_t *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* CRC16_H */
