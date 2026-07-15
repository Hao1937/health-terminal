/**
 * @file    max30102.h
 * @brief   MAX30102 心率血氧（I2C 0x57，采样缓冲交 algorithms/spo2_hr 解算）
 * @owner   王宇浩（组长）
 *
 * 统一测量接口：max30102\_init() 初始化，max30102\_measure() 采一次写入
 * hs_sample_t。 未实现前一律返回 HS_NOT_IMPLEMENTED。
 */
#ifndef MODULE_MAX30102_H
#define MODULE_MAX30102_H

#include "health_if.h"

hs_status_t max30102_init(void);
hs_status_t max30102_measure(hs_sample_t *out);

#endif /* MODULE_MAX30102_H */
