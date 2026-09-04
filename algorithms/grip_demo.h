/**
 * @file    grip_demo.h
 * @brief   中期演示用的确定性握力读数序列（不代表真传感器标定结果）。
 * @owner   王宇浩（组长）
 */
#ifndef ALGORITHMS_GRIP_DEMO_H
#define ALGORITHMS_GRIP_DEMO_H

#include <stdint.h>

/**
 * @brief  返回第 index 次演示测量的握力，单位为 kg×10。
 *
 * 序列固定且循环，便于在无 HX711/测力梁时验证 UI、记录、BLE 与评分链路。
 */
int32_t grip_demo_value(uint32_t index);

#endif /* ALGORITHMS_GRIP_DEMO_H */
