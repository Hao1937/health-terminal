/**
 * @file    height.h
 * @brief   HC-SR04 回波时间到身高的定点数换算（无硬件依赖）。
 */
#ifndef ALGORITHMS_HEIGHT_H
#define ALGORITHMS_HEIGHT_H

#include <stdint.h>

#include "health_if.h"

/**
 * @brief 使用 DS18B20 温度补偿声速，把 HC-SR04 回波宽度换算为身高。
 *
 * 声速模型：c = 331.3 + 0.606*T (m/s)，回波为往返时间。
 *
 * @param echo_us           Echo 高电平宽度，单位 us。
 * @param temperature_x100  温度，单位 0.01 摄氏度。
 * @param install_height_mm 传感器发射面到地面的垂直高度，单位 mm。
 * @param height_mm         输出身高，单位 mm。
 * @param distance_mm       可选输出：传感器到头顶的距离，单位 mm；可为 NULL。
 */
hs_status_t height_from_echo_us(uint32_t echo_us, int32_t temperature_x100,
                                int32_t install_height_mm, int32_t *height_mm,
                                int32_t *distance_mm);

#endif /* ALGORITHMS_HEIGHT_H */
