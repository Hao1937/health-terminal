/**
 * @file    bodyfat_navy.h
 * @brief   BMI 与体脂率（美国海军围度法）计算。
 * @owner   王宇浩（组长）
 *
 * BMI 由身高体重直接得出。体脂率用美国海军公式，需颈围/腰围（女性再加臀围），
 * 这些围度由用户经矩阵键盘输入（本终端不含围度传感器）。全为纯定点/浮点运算，
 * 可在 PC 测试。
 */
#ifndef BODYFAT_NAVY_H
#define BODYFAT_NAVY_H

#include <stdint.h>
#include "health_if.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { SEX_MALE = 0, SEX_FEMALE = 1 } sex_t;

/**
 * @brief 计算 BMI（×100）。
 * @param height_mm 身高 mm
 * @param weight_g  体重 g
 * @param bmi_x100  输出 BMI ×100
 * @return HS_OK / HS_NOT_READY（身高非法）
 */
hs_status_t bmi_compute(int32_t height_mm, int32_t weight_g, int32_t *bmi_x100);

/**
 * @brief 美国海军体脂率公式。
 * @param sex        性别
 * @param height_mm  身高 mm
 * @param neck_mm    颈围 mm
 * @param waist_mm   腰围 mm
 * @param hip_mm     臀围 mm（男性可填 0）
 * @param bodyfat_x10 输出体脂率 ×10（百分数）
 * @return HS_OK / HS_NOT_READY（围度非法导致对数域错误）
 */
hs_status_t bodyfat_navy(sex_t sex, int32_t height_mm, int32_t neck_mm,
                         int32_t waist_mm, int32_t hip_mm, int32_t *bodyfat_x10);

#ifdef __cplusplus
}
#endif

#endif /* BODYFAT_NAVY_H */
