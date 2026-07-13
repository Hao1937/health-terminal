/**
 * @file    ds18b20.h
 * @brief   DS18B20 温度（单总线 PB5），为身高声速与体测环境提供温度
 * @owner   查樊听
 *
 * 统一测量接口：ds18b20\_init() 初始化，ds18b20\_measure() 采一次写入 hs_sample_t。
 * 未实现前一律返回 HS_NOT_IMPLEMENTED。
 */
#ifndef MODULE_DS18B20_H
#define MODULE_DS18B20_H

#include "health_if.h"

hs_status_t ds18b20_init(void);
hs_status_t ds18b20_measure(hs_sample_t *out);

#endif /* MODULE_DS18B20_H */
