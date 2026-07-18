/**
 * @file    spo2_hr.c
 * @owner   王宇浩（组长）
 */
#include "spo2_hr.h"

/* 计算缓冲均值（直流分量） */
static int64_t mean_i32(const int32_t *b, size_t n) {
  int64_t s=0;
  for (size_t i = 0; i < n; ++i) s += b[i];
  return (n > 0) ? s / (int64_t)n : 0;
}

hs_status_t spo2_hr_compute(const int32_t *ir_buf, const int32_t *red_buf,
                            size_t n, uint32_t fs_hz, spo2_hr_result_t *out) {
  if (!ir_buf || !red_buf || !out) return HS_NOT_READY;
  out->valid = 0;
  out->heart_rate_bpm = HS_VALUE_INVALID;
  out->spo2_x10 = HS_VALUE_INVALID;
  if (n < 32 || fs_hz == 0) return HS_NOT_READY;

  int64_t ir_dc = mean_i32(ir_buf, n);
  int64_t red_dc = mean_i32(red_buf, n);
  if (ir_dc <= 0 || red_dc <= 0) return HS_UNSTABLE;

  /* ---- 心率：在去直流的 IR 上做过阈值上升沿峰值检测 ---- */
  /* 阈值取信号峰峰值的一部分，抗噪 */
  int32_t ir_min = ir_buf[0], ir_max = ir_buf[0];
  for (size_t i = 1; i < n; ++i) {
    if (ir_buf[i] < ir_min) ir_min = ir_buf[i];
    if (ir_buf[i] > ir_max) ir_max = ir_buf[i];
  }
  int32_t ir_ac_pp = ir_max - ir_min;
  if (ir_ac_pp < 4) return HS_UNSTABLE;  /* 信号太弱 */
  int64_t thresh = ir_dc + ir_ac_pp / 4; /* 高于直流 1/4 峰峰视为搏动上冲 */

  size_t peak_count = 0;
  size_t first_peak = 0, last_peak = 0;
  int above = 0;
  for (size_t i = 0; i < n; ++i) {
    if (!above && ir_buf[i] > thresh) {
      above = 1;
      if (peak_count == 0) first_peak = i;
      last_peak = i;
      peak_count++;
    } else if (above && ir_buf[i] < ir_dc) {
      above = 0; /* 回落到直流以下，等待下一个搏动 */
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

  /* ---- 血氧：R = (AC_red/DC_red) / (AC_ir/DC_ir)，经验映射 ---- */
  int32_t red_min = red_buf[0], red_max = red_buf[0];
  for (size_t i = 1; i < n; ++i) {
    if (red_buf[i] < red_min) red_min = red_buf[i];
    if (red_buf[i] > red_max) red_max = red_buf[i];
  }
  double ac_red = (double)(red_max - red_min);
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
