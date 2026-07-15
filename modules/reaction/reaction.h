/**
 * @file    reaction.h
 * @brief   反应时间测试（LED=PB1，按键=PB0/EXTI0）
 * @owner   刘晏铭
 *
 * 统一测量接口：reaction\_init() 初始化，reaction\_measure() 采一次写入
 * hs_sample_t。 未实现前一律返回 HS_NOT_IMPLEMENTED。
 */
#ifndef MODULE_REACTION_H
#define MODULE_REACTION_H

#include "health_if.h"

hs_status_t reaction_init(void);
hs_status_t reaction_measure(hs_sample_t *out);

#endif /* MODULE_REACTION_H */
