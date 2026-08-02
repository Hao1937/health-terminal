/**
 * @file    spo2_hr.c
 * @owner   王宇浩（组长）
 */
#include "spo2_hr.h"

#define FINGER_DC_MIN 5000
#define AC_PP_MIN 32

/* 计算缓冲均值（直流分量） */
static int64_t mean_i32(const int32_t *b, size_t n) {
  int64_t s = 0;
  for (size_t i = 0; i < n; ++i) s += b[i];
  return (n > 0) ? s / (int64_t)n : 0;
}

static int64_t window_mean(const int32_t *b, size_t center, size_t radius) {
  int64_t sum = 0;
  size_t first = center - radius;
  size_t last = center + radius;
  for (size_t i = first; i <= last; ++i) sum += b[i];
  return sum / (int64_t)(2u * radius + 1u);
}

/* 短窗保留脉搏，长窗跟踪慢速基线；两者相减可抑制手指压力造成的漂移。 */
static int64_t detrended_at(const int32_t *b, size_t center,
                            size_t short_radius, size_t long_radius) {
  return window_mean(b, center, short_radius) -
         window_mean(b, center, long_radius);
}

static int64_t abs_i64(int64_t v) { return v < 0 ? -v : v; }

hs_status_t spo2_hr_compute(const int32_t *ir_buf, const int32_t *red_buf,
                            size_t n, uint32_t fs_hz, spo2_hr_result_t *out) {
  if (!ir_buf || !red_buf || !out) return HS_NOT_READY;
  out->valid = 0;
  out->heart_rate_bpm = HS_VALUE_INVALID;
  out->spo2_x10 = HS_VALUE_INVALID;
  if (n < 32 || fs_hz == 0) return HS_NOT_READY;

  int64_t ir_dc = mean_i32(ir_buf, n);
  int64_t red_dc = mean_i32(red_buf, n);
  /* 真板空载约数百计数；先拒绝环境光噪声，避免凭噪声算出假心率。 */
  if (ir_dc < FINGER_DC_MIN || red_dc < FINGER_DC_MIN) return HS_UNSTABLE;

  size_t long_radius = fs_hz / 4u; /* 约 0.5s 长窗 */
  if (long_radius < 4u) long_radius = 4u;
  if (2u * long_radius + 3u >= n) long_radius = (n - 3u) / 2u;
  size_t short_radius = fs_hz / 50u; /* 约 5 点短窗（100Hz 时） */
  if (short_radius < 1u) short_radius = 1u;
  if (short_radius >= long_radius) short_radius = 1u;

  size_t begin = long_radius;
  size_t end = n - long_radius;
  int64_t ir_ac_min = detrended_at(ir_buf, begin, short_radius, long_radius);
  int64_t ir_ac_max = ir_ac_min;
  int64_t red_ac_min = detrended_at(red_buf, begin, short_radius, long_radius);
  int64_t red_ac_max = red_ac_min;
  for (size_t i = begin + 1u; i < end; ++i) {
    int64_t ir_ac = detrended_at(ir_buf, i, short_radius, long_radius);
    int64_t red_ac = detrended_at(red_buf, i, short_radius, long_radius);
    if (ir_ac < ir_ac_min) ir_ac_min = ir_ac;
    if (ir_ac > ir_ac_max) ir_ac_max = ir_ac;
    if (red_ac < red_ac_min) red_ac_min = red_ac;
    if (red_ac > red_ac_max) red_ac_max = red_ac;
  }

  int64_t ir_ac_pp = ir_ac_max - ir_ac_min;
  int64_t red_ac_pp = red_ac_max - red_ac_min;
  if (ir_ac_pp < AC_PP_MIN || red_ac_pp <= 0) return HS_UNSTABLE;

  /* 选择波形主方向，并用双阈值与生理最短峰间隔抑制重复计峰。 */
  int polarity = abs_i64(ir_ac_max) >= abs_i64(ir_ac_min) ? 1 : -1;
  int64_t wave_min = polarity > 0 ? ir_ac_min : -ir_ac_max;
  int64_t wave_max = polarity > 0 ? ir_ac_max : -ir_ac_min;
  int64_t high = wave_min + (wave_max - wave_min) * 65 / 100;
  int64_t low = wave_min + (wave_max - wave_min) * 35 / 100;
  size_t min_peak_gap = ((size_t)fs_hz * 60u + 219u) / 220u;
  if (min_peak_gap < 1u) min_peak_gap = 1u;

  size_t peak_count = 0;
  size_t first_peak = 0, last_peak = 0;
  int armed = 1;
  for (size_t i = begin; i < end; ++i) {
    int64_t wave =
        polarity * detrended_at(ir_buf, i, short_radius, long_radius);
    if (armed && wave >= high) {
      if (peak_count == 0 || i - last_peak >= min_peak_gap) {
        if (peak_count == 0) first_peak = i;
        last_peak = i;
        ++peak_count;
      }
      armed = 0;
    } else if (!armed && wave <= low) {
      armed = 1;
    }
  }
  if (peak_count < 2) return HS_UNSTABLE;

  /* 平均峰间隔（样本）→ bpm */
  double intervals =
      (double)(last_peak - first_peak) / (double)(peak_count - 1);
  if (intervals <= 0.0) return HS_UNSTABLE;
  double bpm = 60.0 * (double)fs_hz / intervals;
  if (bpm < 30.0 || bpm > 220.0) return HS_UNSTABLE;
  out->heart_rate_bpm = (int32_t)(bpm + 0.5);

  /* ---- 血氧：使用去趋势后的 AC，避免接触压力漂移污染 R 值 ---- */
  double ac_red = (double)red_ac_pp;
  double ac_ir = (double)ir_ac_pp;
  double r = (ac_red / (double)red_dc) / (ac_ir / (double)ir_dc);
  /* 常用经验公式：SpO2 = 110 - 25*R，夹到合理范围 */
  double spo2 = 110.0 - 25.0 * r;
  if (spo2 > 100.0) spo2 = 100.0;
  if (spo2 < 70.0) spo2 = 70.0;
  out->spo2_x10 = (int32_t)(spo2 * 10.0 + 0.5);

  out->valid = 1;
  return HS_OK;
}
