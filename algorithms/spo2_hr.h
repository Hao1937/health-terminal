/**
 * @file    spo2_hr.h
 * @brief   MAX30102 心率/血氧解算（纯算法，与驱动分离）。
 * @owner   王宇浩（组长）
 *
 * 输入为一段等间隔采样的红光(RED)与红外(IR)原始计数缓冲，输出心率(bpm)与
 * 血氧(SpO2 ×10)。算法：
 *   - 去直流（减滑动均值）后在 IR 通道上做峰值检测估心率；
 *   - 用 RED/IR 的交流/直流比值算 R，经经验二次曲线映射到 SpO2。
 * 无硬件依赖，可在 PC 用录制的样本回放测试。
 */
#ifndef SPO2_HR_H
#define SPO2_HR_H

#include <stdint.h>
#include <stddef.h>
#include "health_if.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int32_t heart_rate_bpm; /* 估计心率 */
    int32_t spo2_x10;       /* 估计血氧 ×10，例 985 = 98.5% */
    uint8_t valid;          /* 1=结果可信，0=信号太弱/峰值不足 */
} spo2_hr_result_t;

/**
 * @brief 从一段样本估计心率与血氧。
 * @param ir_buf   IR 通道样本
 * @param red_buf  RED 通道样本
 * @param n        样本数（建议 ≥ 100，覆盖数个心动周期）
 * @param fs_hz    采样率（Hz），用于把峰间隔换算成 bpm
 * @return HS_OK / HS_UNSTABLE（峰太少）/ HS_NOT_READY（样本不足）
 */
hs_status_t spo2_hr_compute(const int32_t *ir_buf, const int32_t *red_buf,
                            size_t n, uint32_t fs_hz, spo2_hr_result_t *out);

#ifdef __cplusplus
}
#endif

#endif /* SPO2_HR_H */
