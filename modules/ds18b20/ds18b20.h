/**
 * @file    ds18b20.h
 * @brief   DS18B20 温度（单总线 PB5），为身高声速与体测环境提供温度
 * @owner   查樊听
 *
 * ds18b20_measure() 输出 primary=温度(0.01°C)，带 Scratchpad CRC8 校验。
 * 本驱动要求三线供电，不支持寄生供电模式。
 */
#ifndef MODULE_DS18B20_H
#define MODULE_DS18B20_H

#include "health_if.h"

hs_status_t ds18b20_init(void);
hs_status_t ds18b20_measure(hs_sample_t *out);

/** @brief 读取温度，输出单位为 0.01 摄氏度，供 HC-SR04 声速补偿使用。 */
hs_status_t ds18b20_read_temperature(int32_t *temperature_x100);

#endif /* MODULE_DS18B20_H */
