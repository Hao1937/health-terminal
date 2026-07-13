/**
 * @file    balance.h
 * @brief   平衡晃动指数：由一段姿态角序列量化身体摇晃程度。
 * @owner   刘晏铭
 *
 * 输入 MPU6050 互补滤波得到的俯仰/横滚角序列（度，×100 定点），输出晃动指数
 * balance_x10（越小越稳）。指数定义为俯仰与横滚角的样本标准差之和，再折算。
 * 纯算法，可在 PC 用合成序列测试。
 */
#ifndef BALANCE_H
#define BALANCE_H

#include <stdint.h>
#include <stddef.h>
#include "health_if.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 计算平衡晃动指数。
 * @param pitch_x100 俯仰角序列，单位度 ×100
 * @param roll_x100  横滚角序列，单位度 ×100
 * @param n          样本数（建议 ≥ 20）
 * @param out_x10    输出晃动指数 ×10
 * @return HS_OK / HS_NOT_READY（样本不足）
 */
hs_status_t balance_index(const int32_t *pitch_x100, const int32_t *roll_x100,
                          size_t n, int32_t *out_x10);

#ifdef __cplusplus
}
#endif

#endif /* BALANCE_H */
