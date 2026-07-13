/**
 * @file    record_codec.h
 * @brief   measurement_record_t 的 CRC 收尾/校验，以及 UART/BLE 帧编解码。
 * @owner   王宇浩（组长）
 *
 * 存储层（storage）与传输层（ble）共用本文件，web/dashboard.html 的 JS 与之镜像。
 * 纯 C、无硬件依赖，可在 PC 测试帧的往返一致性与 CRC 抗篡改。
 */
#ifndef RECORD_CODEC_H
#define RECORD_CODEC_H

#include <stdint.h>
#include <stddef.h>
#include "health_record.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 填好 version 并计算写入 record->crc16。 */
void record_finalize(measurement_record_t *rec);

/** @brief 校验 record 的 version 与 crc16，正确返回 1。 */
int record_verify(const measurement_record_t *rec);

/**
 * @brief 把一条 record 打包成一帧（FRAME_TYPE_RECORD）写入 out。
 * @param rec   源记录
 * @param out   输出缓冲
 * @param cap   输出缓冲容量
 * @return 写入的字节数；容量不足返回 0。
 */
size_t frame_encode_record(const measurement_record_t *rec, uint8_t *out, size_t cap);

/**
 * @brief 从字节流解析一帧 record。要求 in 指向 SOF0。
 * @param in    输入帧字节
 * @param len   输入长度
 * @param rec   输出记录
 * @return 成功解析返回消耗的字节数；帧头/长度/CRC 错误返回 0。
 */
size_t frame_decode_record(const uint8_t *in, size_t len, measurement_record_t *rec);

#ifdef __cplusplus
}
#endif

#endif /* RECORD_CODEC_H */
