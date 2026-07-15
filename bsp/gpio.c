/**
 * @file    gpio.c
 * @owner   王宇浩（组长）
 */
#include "gpio.h"

#include "board.h"

void board_gpio_init(void) {
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /* 状态 LED：PC13 推挽输出，默认熄灭（高电平） */
  GPIO_InitTypeDef g = {0};
  g.Pin = LED_STATUS_PIN;
  g.Mode = GPIO_MODE_OUTPUT_PP;
  g.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_STATUS_PORT, &g);
  LED_STATUS_OFF();
}
