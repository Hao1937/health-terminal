/**
 * @file    hx711_grip.h
 * @brief   HX711#2 握力（YZC-131 悬臂梁），软件时序，含标定
 * @owner   王宇浩（组长）
 *
 * 统一测量接口：hx711_grip\_init() 初始化，hx711_grip\_measure() 采一次写入
 * hs_sample_t。 未实现前一律返回 HS_NOT_IMPLEMENTED。
 */
#ifndef MODULE_HX711_GRIP_H
#define MODULE_HX711_GRIP_H

#include "health_if.h"

hs_status_t hx711_grip_init(void);
hs_status_t hx711_grip_measure(hs_sample_t *out);

#endif /* MODULE_HX711_GRIP_H */
