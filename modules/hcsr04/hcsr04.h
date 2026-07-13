/**
 * @file    hcsr04.h
 * @brief   HC-SR04 身高（Trig/Echo，TIM4 输入捕获），配合 DS18B20 温度补偿声速
 * @owner   查樊听
 *
 * 统一测量接口：hcsr04\_init() 初始化，hcsr04\_measure() 采一次写入 hs_sample_t。
 * 未实现前一律返回 HS_NOT_IMPLEMENTED。
 */
#ifndef MODULE_HCSR04_H
#define MODULE_HCSR04_H

#include "health_if.h"

hs_status_t hcsr04_init(void);
hs_status_t hcsr04_measure(hs_sample_t *out);

#endif /* MODULE_HCSR04_H */
