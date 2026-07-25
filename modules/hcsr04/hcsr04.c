/**
 * @file    hcsr04.c
 * @brief   HC-SR04 身高测量：PB9 Trig，PB8/TIM4_CH3 Echo。
 * @owner   查樊听
 */
#include "hcsr04.h"

#ifdef USE_HAL_DRIVER
#include "stm32f1xx_hal.h"
TIM_HandleTypeDef *g_hcsr04_htim = 0;
#endif

#if defined(MODULE_ENABLED_HCSR04)

#include "board.h"
#include "delay.h"
#include "ds18b20.h"
#include "height.h"

#ifndef HCSR04_INSTALL_HEIGHT_MM
#define HCSR04_INSTALL_HEIGHT_MM 2000
#endif

#define HCSR04_SAMPLE_COUNT 5U
#define HCSR04_MIN_VALID_SAMPLES 3U
#define HCSR04_ECHO_TIMEOUT_MS 30U
#define HCSR04_SAMPLE_INTERVAL_MS 60U
#define HCSR04_MAX_SPREAD_MM 30

static TIM_HandleTypeDef s_htim4;
static uint8_t s_initialized;

static uint32_t tim4_clock_hz(void) {
  RCC_ClkInitTypeDef clocks;
  uint32_t flash_latency;
  HAL_RCC_GetClockConfig(&clocks, &flash_latency);
  uint32_t clock = HAL_RCC_GetPCLK1Freq();
  if (clocks.APB1CLKDivider != RCC_HCLK_DIV1) {
    clock *= 2U;
  }
  return clock;
}

static uint8_t wait_capture_flag(uint32_t timeout_ms) {
  uint32_t started = HAL_GetTick();
  while (__HAL_TIM_GET_FLAG(&s_htim4, TIM_FLAG_CC3) == RESET) {
    if ((HAL_GetTick() - started) >= timeout_ms) {
      return 0U;
    }
  }
  return 1U;
}

static hs_status_t read_echo_us(uint32_t *echo_us) {
  if (echo_us == NULL) {
    return HS_UNSTABLE;
  }

  /* 等待上一轮 Echo 彻底结束，防止把残留高电平当作新回波。 */
  uint32_t idle_started = HAL_GetTick();
  while (HAL_GPIO_ReadPin(HCSR04_ECHO_PORT, HCSR04_ECHO_PIN) == GPIO_PIN_SET) {
    if ((HAL_GetTick() - idle_started) >= 2U) {
      return HS_NOT_READY;
    }
  }

  __HAL_TIM_SET_COUNTER(&s_htim4, 0U);
  __HAL_TIM_SET_CAPTUREPOLARITY(&s_htim4, TIM_CHANNEL_3,
                                TIM_INPUTCHANNELPOLARITY_RISING);
  __HAL_TIM_CLEAR_FLAG(&s_htim4, TIM_FLAG_CC3);
  if (HAL_TIM_IC_Start(&s_htim4, TIM_CHANNEL_3) != HAL_OK) {
    return HS_NOT_READY;
  }

  HAL_GPIO_WritePin(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN, GPIO_PIN_RESET);
  delay_us(2U);
  HAL_GPIO_WritePin(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN, GPIO_PIN_SET);
  delay_us(10U);
  HAL_GPIO_WritePin(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN, GPIO_PIN_RESET);

  if (wait_capture_flag(HCSR04_ECHO_TIMEOUT_MS) == 0U) {
    (void)HAL_TIM_IC_Stop(&s_htim4, TIM_CHANNEL_3);
    return HS_TIMEOUT;
  }
  uint16_t rising =
      (uint16_t)HAL_TIM_ReadCapturedValue(&s_htim4, TIM_CHANNEL_3);

  __HAL_TIM_SET_CAPTUREPOLARITY(&s_htim4, TIM_CHANNEL_3,
                                TIM_INPUTCHANNELPOLARITY_FALLING);
  __HAL_TIM_CLEAR_FLAG(&s_htim4, TIM_FLAG_CC3);
  if (wait_capture_flag(HCSR04_ECHO_TIMEOUT_MS) == 0U) {
    (void)HAL_TIM_IC_Stop(&s_htim4, TIM_CHANNEL_3);
    return HS_TIMEOUT;
  }
  uint16_t falling =
      (uint16_t)HAL_TIM_ReadCapturedValue(&s_htim4, TIM_CHANNEL_3);
  (void)HAL_TIM_IC_Stop(&s_htim4, TIM_CHANNEL_3);

  /* TIM4 为 16 bit、1MHz；无符号减法自然处理一次计数回绕。 */
  *echo_us = (uint16_t)(falling - rising);
  return (*echo_us >= 100U && *echo_us <= 30000U) ? HS_OK : HS_UNSTABLE;
}

static void sort_i32(int32_t *values, uint8_t count) {
  for (uint8_t i = 1U; i < count; ++i) {
    int32_t value = values[i];
    uint8_t j = i;
    while (j > 0U && values[j - 1U] > value) {
      values[j] = values[j - 1U];
      --j;
    }
    values[j] = value;
  }
}

hs_status_t hcsr04_init(void) {
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_TIM4_CLK_ENABLE();

  GPIO_InitTypeDef gpio = {0};
  gpio.Pin = HCSR04_TRIG_PIN;
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(HCSR04_TRIG_PORT, &gpio);
  HAL_GPIO_WritePin(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN, GPIO_PIN_RESET);

  gpio.Pin = HCSR04_ECHO_PIN;
  gpio.Mode = GPIO_MODE_INPUT;
  gpio.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(HCSR04_ECHO_PORT, &gpio);

  s_htim4.Instance = TIM4;
  s_htim4.Init.Prescaler = (tim4_clock_hz() / 1000000U) - 1U;
  s_htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  s_htim4.Init.Period = 0xFFFFU;
  s_htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  s_htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_IC_Init(&s_htim4) != HAL_OK) {
    return HS_NOT_READY;
  }

  TIM_IC_InitTypeDef capture = {0};
  capture.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  capture.ICSelection = TIM_ICSELECTION_DIRECTTI;
  capture.ICPrescaler = TIM_ICPSC_DIV1;
  capture.ICFilter = 4U;
  if (HAL_TIM_IC_ConfigChannel(&s_htim4, &capture, TIM_CHANNEL_3) != HAL_OK) {
    return HS_NOT_READY;
  }

  g_hcsr04_htim = &s_htim4;
  hs_status_t temperature_status = ds18b20_init();
  s_initialized = (temperature_status == HS_OK);
  return temperature_status;
}

hs_status_t hcsr04_measure(hs_sample_t *out) {
  if (out == NULL) {
    return HS_UNSTABLE;
  }
  if (s_initialized == 0U) {
    hs_status_t status = hcsr04_init();
    if (status != HS_OK) {
      return status;
    }
  }

  int32_t temperature_x100;
  hs_status_t temperature_status = ds18b20_read_temperature(&temperature_x100);
  if (temperature_status != HS_OK) {
    return temperature_status;
  }

  int32_t heights[HCSR04_SAMPLE_COUNT];
  uint8_t valid = 0U;
  for (uint8_t i = 0U; i < HCSR04_SAMPLE_COUNT; ++i) {
    uint32_t echo_us;
    if (read_echo_us(&echo_us) == HS_OK) {
      int32_t height_mm;
      if (height_from_echo_us(echo_us, temperature_x100,
                              HCSR04_INSTALL_HEIGHT_MM, &height_mm,
                              NULL) == HS_OK) {
        heights[valid++] = height_mm;
      }
    }
    if (i + 1U < HCSR04_SAMPLE_COUNT) {
      delay_ms(HCSR04_SAMPLE_INTERVAL_MS);
    }
  }

  if (valid < HCSR04_MIN_VALID_SAMPLES) {
    return HS_TIMEOUT;
  }
  sort_i32(heights, valid);
  if ((heights[valid - 1U] - heights[0]) > HCSR04_MAX_SPREAD_MM) {
    return HS_UNSTABLE;
  }

  out->primary = heights[valid / 2U];
  out->secondary =
      temperature_x100; /* 调试用：同次测量环境温度，单位 0.01°C。 */
  return HS_OK;
}

#else

hs_status_t hcsr04_init(void) { return HS_NOT_IMPLEMENTED; }
hs_status_t hcsr04_measure(hs_sample_t *out) {
  (void)out;
  return HS_NOT_IMPLEMENTED;
}

#endif
