/**
 * @file    delay.c
 * @owner   王宇浩（组长）
 */
#include "delay.h"

#include "stm32f1xx_hal.h"

void delay_init(void) {
  /* 使能 DWT 周期计数器（Cortex-M3 内建） */
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void delay_us(uint32_t us) {
  uint32_t start = DWT->CYCCNT;
  uint32_t ticks = us * (SystemCoreClock / 1000000U);
  while ((DWT->CYCCNT - start) < ticks) {
  }
}

void delay_ms(uint32_t ms) {
  while (ms--) {
    delay_us(1000);
  }
}
