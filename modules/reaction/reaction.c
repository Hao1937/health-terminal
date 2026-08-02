/**
 * @file    reaction.c
 * @owner   刘晏铭
 *
 * 反应时间：PB1 点亮提示 LED，PB0 上拉按键走 EXTI0。测量流程为先随机等待
 * 一小段时间，等待期间按键视为抢跑；LED 亮起时记录 t0，中断回调记录 t1，
 * out->primary 返回 t1-t0(ms)。
 */
#include "reaction.h"

#if defined(MODULE_ENABLED_REACTION)

#include <stdio.h>

#include "board.h"
#include "oled.h"
#include "stm32f1xx_hal.h"

#define REACTION_FALSE_DELAY_MIN_MS 1000u
#define REACTION_FALSE_DELAY_SPAN_MS 2000u
#define REACTION_TIMEOUT_MS 3000u
#define REACTION_DEBOUNCE_MS 20u
#define REACTION_RESULT_HOLD_MS 1200u
#define REACTION_TREND_HOLD_MS 2000u
#define REACTION_TREND_SAMPLES 16u

static int32_t s_rt_history[REACTION_TREND_SAMPLES];
static uint8_t s_rt_count;

static volatile uint8_t s_waiting;
static volatile uint8_t s_led_on;
static volatile uint8_t s_pressed;
static volatile uint8_t s_false_start;
static volatile uint32_t s_press_tick;
static volatile uint32_t s_last_irq_tick;
static uint8_t s_inited;

static void reaction_led_on(void) {
  HAL_GPIO_WritePin(REACTION_LED_PORT, REACTION_LED_PIN, GPIO_PIN_SET);
}

static void reaction_led_off(void) {
  HAL_GPIO_WritePin(REACTION_LED_PORT, REACTION_LED_PIN, GPIO_PIN_RESET);
}

static int reaction_button_down(void) {
  return HAL_GPIO_ReadPin(REACTION_BTN_PORT, REACTION_BTN_PIN) ==
         GPIO_PIN_RESET;
}

static void reaction_show_ready(void) {
  oled_clear();
  oled_show_text(0, 0, "REACTION TEST");
  oled_show_text(0, 2, "WAIT FOR LED");
  oled_show_text(0, 3, "DO NOT PRESS");
  oled_show_text(0, 5, "PB1 LED  PB0 KEY");
}

static void reaction_show_go(void) {
  oled_clear();
  oled_show_text(0, 0, "LED ON");
  oled_show_text(0, 1, "PRESS NOW");
  oled_show_text(0, 3, "OOOOOOOOOOOOOOOOOOOO");
  oled_show_text(0, 4, "OOOOOOOOOOOOOOOOOOOO");
}

static void reaction_show_result(int32_t ms) {
  char line[24];
  oled_clear();
  oled_show_text(0, 0, "REACTION OK");
  snprintf(line, sizeof(line), "TIME:%ldMS", (long)ms);
  oled_show_text(0, 2, line);
  if (ms < 150) {
    oled_show_text(0, 4, "FAST");
  } else if (ms <= 300) {
    oled_show_text(0, 4, "GOOD");
  } else {
    oled_show_text(0, 4, "SLOW");
  }
}

static void reaction_add_history(int32_t ms) {
  if (s_rt_count < REACTION_TREND_SAMPLES) {
    s_rt_history[s_rt_count++] = ms;
    return;
  }
  for (uint8_t i = 1; i < REACTION_TREND_SAMPLES; ++i) {
    s_rt_history[i - 1] = s_rt_history[i];
  }
  s_rt_history[REACTION_TREND_SAMPLES - 1u] = ms;
}

static void reaction_show_trend(void) {
  char line[24];
  oled_clear();
  if (s_rt_count < 2u) {
    oled_show_text(0, 0, "TREND NEED 2");
    oled_show_text(0, 2, "TEST AGAIN");
    return;
  }
  oled_show_trend(s_rt_history, s_rt_count);
  snprintf(line, sizeof(line), "RT TREND N:%u", (unsigned)s_rt_count);
  oled_show_text(0, 0, line);
  oled_show_text(0, 7, "LOWER IS BETTER");
}

static void reaction_show_false_start(void) {
  oled_clear();
  oled_show_text(0, 0, "FALSE START");
  oled_show_text(0, 2, "PRESS AFTER LED");
  oled_show_text(0, 4, "TRY AGAIN");
}

static void reaction_show_timeout(void) {
  oled_clear();
  oled_show_text(0, 0, "TIMEOUT");
  oled_show_text(0, 2, "NO BUTTON PRESS");
  oled_show_text(0, 4, "TRY AGAIN");
}

static void reaction_show_release(void) {
  oled_clear();
  oled_show_text(0, 0, "RELEASE BUTTON");
  oled_show_text(0, 2, "THEN TEST STARTS");
}

hs_status_t reaction_init(void) {
  GPIO_InitTypeDef g = {0};

  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_AFIO_CLK_ENABLE();

  g.Pin = REACTION_LED_PIN;
  g.Mode = GPIO_MODE_OUTPUT_PP;
  g.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(REACTION_LED_PORT, &g);
  reaction_led_off();

  g.Pin = REACTION_BTN_PIN;
  g.Mode = GPIO_MODE_IT_FALLING;
  g.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(REACTION_BTN_PORT, &g);

  __HAL_GPIO_EXTI_CLEAR_IT(REACTION_BTN_PIN);
  HAL_NVIC_SetPriority(REACTION_BTN_EXTI, 1, 0);
  HAL_NVIC_EnableIRQ(REACTION_BTN_EXTI);

  s_waiting = 0;
  s_led_on = 0;
  s_pressed = 0;
  s_false_start = 0;
  s_press_tick = 0;
  s_last_irq_tick = 0;
  s_inited = 1;
  return HS_OK;
}

hs_status_t reaction_measure(hs_sample_t *out) {
  if (out == NULL) return HS_NOT_READY;
  if (!s_inited) {
    hs_status_t st = reaction_init();
    if (st != HS_OK) return st;
  }

  out->primary = HS_VALUE_INVALID;
  out->secondary = HS_VALUE_INVALID;
  reaction_led_off();

  if (reaction_button_down()) {
    reaction_show_release();
    uint32_t release_t0 = HAL_GetTick();
    while (reaction_button_down()) {
      if (HAL_GetTick() - release_t0 > REACTION_TIMEOUT_MS) {
        reaction_show_false_start();
        HAL_Delay(REACTION_RESULT_HOLD_MS);
        return HS_UNSTABLE;
      }
    }
    HAL_Delay(100);
  }

  s_waiting = 1;
  s_led_on = 0;
  s_pressed = 0;
  s_false_start = 0;
  __HAL_GPIO_EXTI_CLEAR_IT(REACTION_BTN_PIN);

  reaction_show_ready();
  uint32_t wait_ms = REACTION_FALSE_DELAY_MIN_MS +
                     (HAL_GetTick() % REACTION_FALSE_DELAY_SPAN_MS);
  uint32_t wait_t0 = HAL_GetTick();
  while (HAL_GetTick() - wait_t0 < wait_ms) {
    if (s_false_start || reaction_button_down()) {
      s_waiting = 0;
      reaction_led_off();
      reaction_show_false_start();
      HAL_Delay(REACTION_RESULT_HOLD_MS);
      return HS_UNSTABLE;
    }
  }

  uint32_t t0 = HAL_GetTick();
  s_led_on = 1;
  reaction_led_on();
  reaction_show_go();

  while (!s_pressed) {
    if (HAL_GetTick() - t0 > REACTION_TIMEOUT_MS) {
      s_waiting = 0;
      s_led_on = 0;
      reaction_led_off();
      reaction_show_timeout();
      HAL_Delay(REACTION_RESULT_HOLD_MS);
      return HS_TIMEOUT;
    }
  }

  s_waiting = 0;
  s_led_on = 0;
  reaction_led_off();

  int32_t rt_ms = (int32_t)(s_press_tick - t0);
  out->primary = rt_ms;
  reaction_add_history(rt_ms);
  reaction_show_result(rt_ms);
  HAL_Delay(REACTION_RESULT_HOLD_MS);
  reaction_show_trend();
  HAL_Delay(REACTION_TREND_HOLD_MS);
  return HS_OK;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  if (GPIO_Pin != REACTION_BTN_PIN || !s_waiting) return;

  uint32_t now = HAL_GetTick();
  if (now - s_last_irq_tick < REACTION_DEBOUNCE_MS) return;
  s_last_irq_tick = now;

  if (!s_led_on) {
    s_false_start = 1;
    return;
  }

  if (!s_pressed) {
    s_press_tick = now;
    s_pressed = 1;
  }
}

#else /* 本板 MODULE_SET 未包含该模块：桩替代，仅供 app 注册表链接 */

hs_status_t reaction_init(void) { return HS_NOT_IMPLEMENTED; }
hs_status_t reaction_measure(hs_sample_t *out) {
  (void)out;
  return HS_NOT_IMPLEMENTED;
}

#endif
