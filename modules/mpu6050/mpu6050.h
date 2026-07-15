/**
 * @file    mpu6050.h
 * @brief   MPU6050 平衡晃动指数（I2C 0x68，互补滤波+balance）
 * @owner   刘晏铭
 *
 * 统一测量接口：mpu6050\_init() 初始化，mpu6050\_measure() 采一次写入
 * hs_sample_t。 未实现前一律返回 HS_NOT_IMPLEMENTED。
 */
#ifndef MODULE_MPU6050_H
#define MODULE_MPU6050_H

#include "health_if.h"

hs_status_t mpu6050_init(void);
hs_status_t mpu6050_measure(hs_sample_t *out);

#endif /* MODULE_MPU6050_H */
