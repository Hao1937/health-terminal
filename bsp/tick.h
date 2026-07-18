/**
 * @file    tick.h
 * @brief   系统毫秒时间戳（封装 HAL_GetTick，供上层做超时判断）。
 * @owner   王宇浩（组长）
 *
 * 上层模块（如 max30102）需要「记录起始时刻、判断是否超时」时，
 * 经本封装读取时间戳，不直接碰 HAL，便于 host 端用 mock 测试超时分支。
 */
#ifndef BSP_TICK_H
#define BSP_TICK_H

#include <stdint.h>

/** @brief 读取系统运行毫秒数（自上电/复位起累计）。 */
uint32_t tick_now_ms(void);

#endif /* BSP_TICK_H */
