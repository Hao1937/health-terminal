/**
 * @file    stm32f1xx_hal_conf.h
 * @brief   STM32F1xx HAL 配置（裁剪版，只开启本项目用到的模块以控制体积）。
 * @owner   王宇浩（组长）
 *
 * 与 CubeMX 生成结构兼容：如后续改用 CubeMX，可用其生成的同名文件替换本文件。
 */
#ifndef STM32F1xx_HAL_CONF_H
#define STM32F1xx_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/* ---- 启用的 HAL 模块（对应 firmware.cmake 里编译的 HAL_SOURCES）---- */
#define HAL_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED

/* ---- 振荡器 / 电压 ---- */
#if !defined  (HSE_VALUE)
  #define HSE_VALUE               8000000U  /* 最小系统板 8MHz 外部晶振 */
#endif
#define HSE_STARTUP_TIMEOUT       100U
#define HSI_VALUE                 8000000U
#define LSI_VALUE                 40000U
#define LSE_VALUE                 32768U
#define LSE_STARTUP_TIMEOUT       5000U
#define VDD_VALUE                 3300U
#define TICK_INT_PRIORITY         15U
#define USE_RTOS                  0U
#define PREFETCH_ENABLE           1U

#define USE_HAL_I2C_REGISTER_CALLBACKS  0U
#define USE_HAL_UART_REGISTER_CALLBACKS 0U
#define USE_HAL_TIM_REGISTER_CALLBACKS  0U

/* ---- 断言（发布版关闭以省体积）---- */
/* #define USE_FULL_ASSERT   1U */

/* ---- 包含启用模块的头 ---- */
#ifdef HAL_RCC_MODULE_ENABLED
  #include "stm32f1xx_hal_rcc.h"
#endif
#ifdef HAL_GPIO_MODULE_ENABLED
  #include "stm32f1xx_hal_gpio.h"
#endif
#ifdef HAL_DMA_MODULE_ENABLED
  #include "stm32f1xx_hal_dma.h"
#endif
#ifdef HAL_CORTEX_MODULE_ENABLED
  #include "stm32f1xx_hal_cortex.h"
#endif
#ifdef HAL_FLASH_MODULE_ENABLED
  #include "stm32f1xx_hal_flash.h"
#endif
#ifdef HAL_I2C_MODULE_ENABLED
  #include "stm32f1xx_hal_i2c.h"
#endif
#ifdef HAL_UART_MODULE_ENABLED
  #include "stm32f1xx_hal_uart.h"
#endif
#ifdef HAL_TIM_MODULE_ENABLED
  #include "stm32f1xx_hal_tim.h"
#endif

#ifdef USE_FULL_ASSERT
  #define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
  void assert_failed(uint8_t *file, uint32_t line);
#else
  #define assert_param(expr) ((void)0U)
#endif

#ifdef __cplusplus
}
#endif

#endif /* STM32F1xx_HAL_CONF_H */
