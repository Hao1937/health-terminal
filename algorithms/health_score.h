/**
 * @file    health_score.h
 * @brief   综合健康评分：把多项指标折算成 0~100 分。
 * @owner   王宇浩（组长）
 *
 * 对 BMI、体脂率、心率、血氧、平衡、反应时间分别按「理想区间」打分再加权求和。
 * 纯函数，输入一条 measurement_record_t，输出 0~100
 * 整数分。无效指标(HS_VALUE_INVALID) 不参与，权重按有效项重新归一。可在 PC
 * 测试。
 */
#ifndef HEALTH_SCORE_H
#define HEALTH_SCORE_H

#include <stdint.h>

#include "health_if.h"
#include "health_record.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 依据记录中的指标计算综合评分。
 * @param rec       已填好各指标的记录（score/crc 可留空）
 * @param out_score 输出 0~100
 * @return HS_OK / HS_NOT_READY（无任何有效指标）
 */
hs_status_t health_score_compute(const measurement_record_t *rec,
                                 int32_t *out_score);

#ifdef __cplusplus
}
#endif

#endif /* HEALTH_SCORE_H */
