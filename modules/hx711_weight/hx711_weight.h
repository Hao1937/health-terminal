/**
 * @file    hx711_weight.h
 * @brief   HX711#1 体重（50kg×4 全桥），软件时序，含标定
 * @owner   查樊听
 *
 * 统一测量接口：hx711_weight\_init() 初始化，hx711_weight\_measure() 采一次写入
 * hs_sample_t。 未实现前一律返回 HS_NOT_IMPLEMENTED。
 */
#ifndef MODULE_HX711_WEIGHT_H
#define MODULE_HX711_WEIGHT_H

#include "health_if.h"

hs_status_t hx711_weight_init(void);
hs_status_t hx711_weight_measure(hs_sample_t *out);

#endif /* MODULE_HX711_WEIGHT_H */
