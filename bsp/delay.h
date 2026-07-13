/**
 * @file    delay.h
 * @brief   微秒/毫秒延时（基于 Cortex-M3 DWT 周期计数器）。
 * @owner   王宇浩（组长）
 *
 * HX711、DS18B20、HC-SR04 的软件时序都需要精确 us 级延时。
 */
#ifndef BSP_DELAY_H
#define BSP_DELAY_H

#include <stdint.h>

/** @brief 初始化 DWT 计数器，须在 SystemClock_Config 之后调用。 */
void delay_init(void);
/** @brief 忙等 us 微秒（受总线频率限制，适合 <100us 的时序）。 */
void delay_us(uint32_t us);
/** @brief 忙等 ms 毫秒。 */
void delay_ms(uint32_t ms);

#endif /* BSP_DELAY_H */
