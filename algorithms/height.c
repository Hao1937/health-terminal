/**
 * @file    height.c
 * @brief   HC-SR04 身高换算纯算法。
 */
#include "height.h"

#include <stddef.h>

hs_status_t height_from_echo_us(uint32_t echo_us, int32_t temperature_x100,
                                int32_t install_height_mm, int32_t *height_mm,
                                int32_t *distance_mm) {
  if (height_mm == NULL || echo_us == 0U) {
    return HS_TIMEOUT;
  }
  if (temperature_x100 < -5500 || temperature_x100 > 12500 ||
      install_height_mm < 500 || install_height_mm > 3000 || echo_us < 100U ||
      echo_us > 30000U) {
    return HS_UNSTABLE;
  }

  /* mm/s；全程整数运算，0.606 * T(摄氏度) = 606 * T_x100 / 100。 */
  int32_t sound_speed_mm_s = 331300 + (606 * temperature_x100) / 100;
  int32_t measured_distance_mm =
      (int32_t)(((uint64_t)echo_us * (uint32_t)sound_speed_mm_s + 1000000ULL) /
                2000000ULL);
  int32_t measured_height_mm = install_height_mm - measured_distance_mm;

  /* 排除无目标、目标越过安装面以及明显非人体结果。 */
  if (measured_distance_mm < 20 || measured_height_mm < 300 ||
      measured_height_mm >= install_height_mm) {
    return HS_UNSTABLE;
  }

  *height_mm = measured_height_mm;
  if (distance_mm != NULL) {
    *distance_mm = measured_distance_mm;
  }
  return HS_OK;
}
