/**
 * @file    i2c_bus.h
 * @brief   I2C1 共享总线（OLED/MPU6050/MAX30102 共用），带地址读写封装。
 * @owner   王宇浩（组长）
 *
 * 三个从机共用一条 I2C1，靠 7 位地址区分。所有 I2C 从机模块都经本封装访问，
 * 便于统一超时与错误处理，避免各模块各写一套。
 */
#ifndef BSP_I2C_BUS_H
#define BSP_I2C_BUS_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

extern I2C_HandleTypeDef g_sensor_i2c;

/** @brief 初始化 I2C1（100kHz 标准模式，PB6/PB7）。 */
void i2c_bus_init(void);
/** @brief 向 dev_addr(已左移) 的 reg 写 len 字节。返回 HAL_StatusTypeDef。 */
int i2c_mem_write(uint16_t dev_addr, uint16_t reg, const uint8_t *buf, uint16_t len);
/** @brief 从 dev_addr(已左移) 的 reg 读 len 字节。返回 HAL_StatusTypeDef。 */
int i2c_mem_read(uint16_t dev_addr, uint16_t reg, uint8_t *buf, uint16_t len);
/** @brief 探测某地址从机是否在线（ACK）。返回 1 在线、0 不在线。 */
int i2c_probe(uint16_t dev_addr);

#endif /* BSP_I2C_BUS_H */
