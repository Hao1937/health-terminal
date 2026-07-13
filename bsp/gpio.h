/**
 * @file    gpio.h
 * @brief   公共 GPIO：状态 LED、以及各端口时钟统一使能。
 * @owner   王宇浩（组长）
 */
#ifndef BSP_GPIO_H
#define BSP_GPIO_H

#include "stm32f1xx_hal.h"

/** @brief 使能 GPIOA/B/C 时钟并初始化状态 LED(PC13)。 */
void board_gpio_init(void);

#endif /* BSP_GPIO_H */
