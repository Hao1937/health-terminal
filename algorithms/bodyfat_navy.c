/**
 * @file    bodyfat_navy.c
 * @owner   王宇浩（组长）
 */
#include "bodyfat_navy.h"

#include <math.h>

hs_status_t bmi_compute(int32_t height_mm, int32_t weight_g,
                        int32_t *bmi_x100) {
  if (!bmi_x100) return HS_NOT_READY;
  if (height_mm < 500 || height_mm > 2500) return HS_NOT_READY; /* 0.5~2.5m */
  double h_m = height_mm / 1000.0;
  double w_kg = weight_g / 1000.0;
  double bmi = w_kg / (h_m * h_m);
  *bmi_x100 = (int32_t)(bmi * 100.0 + 0.5);
  return HS_OK;
}

hs_status_t bodyfat_navy(sex_t sex, int32_t height_mm, int32_t neck_mm,
                         int32_t waist_mm, int32_t hip_mm,
                         int32_t *bodyfat_x10) {
  if (!bodyfat_x10) return HS_NOT_READY;
  /* 换算为 cm（海军公式以 cm 为单位） */
  double height = height_mm / 10.0;
  double neck = neck_mm / 10.0;
  double waist = waist_mm / 10.0;
  double hip = hip_mm / 10.0;
  double bf;

  if (sex == SEX_MALE) {
    double d = waist - neck;
    if (d <= 0.0 || height <= 0.0) return HS_NOT_READY;
    bf =
        495.0 / (1.0324 - 0.19077 * log10(d) + 0.15456 * log10(height)) - 450.0;
  } else {
    double d = waist + hip - neck;
    if (d <= 0.0 || height <= 0.0) return HS_NOT_READY;
    bf = 495.0 / (1.29579 - 0.35004 * log10(d) + 0.22100 * log10(height)) -
         450.0;
  }
  if (bf < 2.0) bf = 2.0;
  if (bf > 60.0) bf = 60.0;
  *bodyfat_x10 = (int32_t)(bf * 10.0 + 0.5);
  return HS_OK;
}
