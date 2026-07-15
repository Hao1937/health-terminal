/**
 * @file    health_score.c
 * @owner   王宇浩（组长）
 */
#include "health_score.h"

/* 三角形隶属度打分：value 落在 [lo,hi] 得满分，越远越低，clamp 到 [0,100] */
static int sub_score_range(int32_t value, int32_t lo, int32_t hi,
                           int32_t span) {
  if (value == HS_VALUE_INVALID) return -1; /* 无效项 */
  if (value >= lo && value <= hi) return 100;
  int32_t dist = (value < lo) ? (lo - value) : (value - hi);
  if (span <= 0) return 0;
  int s = 100 - (int)((int64_t)dist * 100 / span);
  if (s < 0) s = 0;
  return s;
}

hs_status_t health_score_compute(const measurement_record_t *rec,
                                 int32_t *out_score) {
  if (!rec || !out_score) return HS_NOT_READY;

  struct {
    int score;
    int weight;
  } items[6];
  int nitems = 0;

  /* BMI 理想 18.5~24.9 (×100)，span 800 */
  items[nitems].score = sub_score_range(rec->bmi_x100, 1850, 2490, 800);
  items[nitems].weight = 25;
  nitems++;
  /* 体脂率理想 10~20% (×10)，span 150 */
  items[nitems].score = sub_score_range(rec->bodyfat_x10, 100, 200, 150);
  items[nitems].weight = 15;
  nitems++;
  /* 静息心率理想 60~90 bpm，span 40 */
  items[nitems].score = sub_score_range(rec->heart_rate_bpm, 60, 90, 40);
  items[nitems].weight = 20;
  nitems++;
  /* 血氧理想 95~100% (×10)，span 60 */
  items[nitems].score = sub_score_range(rec->spo2_x10, 950, 1000, 60);
  items[nitems].weight = 20;
  nitems++;
  /* 平衡晃动指数越小越好，理想 0~30 (×10)，span 100 */
  items[nitems].score = sub_score_range(rec->balance_x10, 0, 30, 100);
  items[nitems].weight = 10;
  nitems++;
  /* 反应时间理想 150~300ms，span 300 */
  items[nitems].score = sub_score_range(rec->reaction_ms, 150, 300, 300);
  items[nitems].weight = 10;
  nitems++;

  int64_t wsum = 0, acc = 0;
  for (int i = 0; i < nitems; ++i) {
    if (items[i].score < 0) continue; /* 跳过无效项 */
    acc += (int64_t)items[i].score * items[i].weight;
    wsum += items[i].weight;
  }
  if (wsum == 0) return HS_NOT_READY;

  *out_score = (int32_t)((acc + wsum / 2) / wsum);
  return HS_OK;
}
