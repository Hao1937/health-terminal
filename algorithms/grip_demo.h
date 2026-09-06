/**
 * @file    grip_demo.h
 * @brief   30.0~40.0kg 的确定性假数据序列（不代表真传感器测量结果）。
 * @owner   王宇浩（组长）
 */
#ifndef ALGORITHMS_GRIP_DEMO_H
#define ALGORITHMS_GRIP_DEMO_H

#include <stdint.h>

/**
 * @brief  返回第 index 次演示测量的握力，单位为 kg×10。
 *
 * 序列按 0.1kg 步进、每 101 次循环，便于在无 HX711/测力梁时验证
 * UI、记录、BLE 与评分链路。
 */
int32_t grip_demo_value(uint32_t index);

#endif /* ALGORITHMS_GRIP_DEMO_H */
