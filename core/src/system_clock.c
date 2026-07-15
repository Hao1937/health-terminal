/**
 * @file    system_clock.c
 * @brief   系统时钟配置：8MHz HSE ×9 → 72MHz SYSCLK。
 * @owner   王宇浩（组长）
 */
#include "stm32f1xx_hal.h"

/**
 * @brief 配置到 72MHz：HSE 8MHz → PLL×9 = 72MHz；APB1=36MHz、APB2=72MHz。
 *        失败时死循环（早期无外设，靠断点定位）。
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef osc = {0};
  RCC_ClkInitTypeDef clk = {0};

  osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  osc.HSEState = RCC_HSE_ON;
  osc.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  osc.PLL.PLLState = RCC_PLL_ON;
  osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  osc.PLL.PLLMUL = RCC_PLL_MUL9; /* 8MHz × 9 = 72MHz */
  if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
    while (1) {
    }
  }

  clk.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  clk.AHBCLKDivider = RCC_SYSCLK_DIV1; /* HCLK  = 72MHz */
  clk.APB1CLKDivider = RCC_HCLK_DIV2;  /* PCLK1 = 36MHz（上限） */
  clk.APB2CLKDivider = RCC_HCLK_DIV1;  /* PCLK2 = 72MHz */
  /* 72MHz 需 2 个 Flash 等待周期 */
  if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2) != HAL_OK) {
    while (1) {
    }
  }
}
