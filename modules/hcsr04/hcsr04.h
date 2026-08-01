/**
 * @file    hcsr04.h
 * @brief   HC-SR04 身高（Trig/Echo，TIM4 输入捕获），配合 DS18B20 温度补偿声速
 * @owner   查樊听
 *
 * hcsr04_measure() 输出 primary=身高(mm)，secondary=同次测量温度(0.01°C)。
 * 安装高度由 CMake 参数 HCSR04_INSTALL_HEIGHT_MM 配置。
 */
#ifndef MODULE_HCSR04_H
#define MODULE_HCSR04_H

#include "health_if.h"

hs_status_t hcsr04_init(void);
hs_status_t hcsr04_measure(hs_sample_t *out);

#endif /* MODULE_HCSR04_H */
