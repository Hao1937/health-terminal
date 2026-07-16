/**
 * @file    tick.c
 * @owner   王宇浩（组长）
 */
#include "tick.h"

#include "stm32f1xx_hal.h"

uint32_t tick_now_ms(void) { return HAL_GetTick(); }
