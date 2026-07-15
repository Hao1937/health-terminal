/**
 * @file    app_statemachine.c
 * @brief   裸机主状态机：自检→菜单→测量→结果→历史（无 RTOS，主循环驱动）。
 * @owner   王宇浩（组长）
 *
 * 遍历 g_hs_registry 采集各项，未实现的模块返回 HS_NOT_IMPLEMENTED 会被跳过，
 * 因此当前全桩状态下也能稳定空转，模块逐个替换为真实现即可点亮对应指标。
 */
#include <stdio.h>

#include "app.h"
#include "ble.h"
#include "board.h"
#include "bodyfat_navy.h"
#include "delay.h"
#include "gpio.h"
#include "health_if.h"
#include "health_score.h"
#include "i2c_bus.h"
#include "keypad.h"
#include "oled.h"
#include "record_codec.h"
#include "storage.h"
#include "uart.h"

measurement_record_t g_current_record;
static app_state_t s_state;
static uint32_t s_tick_sec;

/* 把注册表某项的采样结果落到记录对应字段 */
static void store_sample(hs_item_t item, const hs_sample_t *s) {
  switch (item) {
    case HS_ITEM_HEIGHT:
      g_current_record.height_mm = s->primary;
      break;
    case HS_ITEM_WEIGHT:
      g_current_record.weight_g = s->primary;
      break;
    case HS_ITEM_HR_SPO2:
      g_current_record.heart_rate_bpm = s->primary;
      g_current_record.spo2_x10 = s->secondary;
      break;
    case HS_ITEM_BALANCE:
      g_current_record.balance_x10 = s->primary;
      break;
    case HS_ITEM_GRIP:
      g_current_record.grip_kg_x10 = s->primary;
      break;
    case HS_ITEM_REACTION:
      g_current_record.reaction_ms = s->primary;
      break;
    default:
      break;
  }
}

static void record_reset(void) {
  g_current_record.timestamp = s_tick_sec;
  g_current_record.height_mm = HS_VALUE_INVALID;
  g_current_record.weight_g = HS_VALUE_INVALID;
  g_current_record.bmi_x100 = HS_VALUE_INVALID;
  g_current_record.bodyfat_x10 = HS_VALUE_INVALID;
  g_current_record.heart_rate_bpm = HS_VALUE_INVALID;
  g_current_record.spo2_x10 = HS_VALUE_INVALID;
  g_current_record.balance_x10 = HS_VALUE_INVALID;
  g_current_record.grip_kg_x10 = HS_VALUE_INVALID;
  g_current_record.reaction_ms = HS_VALUE_INVALID;
  g_current_record.score = HS_VALUE_INVALID;
}

/* 采集全部已实现的测量项 */
static void measure_all(void) {
  for (const hs_sensor_t *s = g_hs_registry; s->measure != NULL; ++s) {
    if (s->init) (void)s->init();
    hs_sample_t sample = {HS_VALUE_INVALID, HS_VALUE_INVALID};
    hs_status_t st = s->measure(&sample);
    printf("[measure] %-9s (%s) -> %s\r\n", s->name, s->owner,
           hs_status_str(st));
    if (st == HS_OK) store_sample(s->item, &sample);
  }

  /* BODY：身高体重齐了就算 BMI（体脂率需围度，暂缺则留空） */
  if (g_current_record.height_mm != HS_VALUE_INVALID &&
      g_current_record.weight_g != HS_VALUE_INVALID) {
    bmi_compute(g_current_record.height_mm, g_current_record.weight_g,
                &g_current_record.bmi_x100);
  }

  int32_t score;
  if (health_score_compute(&g_current_record, &score) == HS_OK) {
    g_current_record.score = score;
  }
  record_finalize(&g_current_record);
}

void app_init(void) {
  board_gpio_init();
  delay_init();
  uart_init();
  i2c_bus_init();
  (void)oled_init();
  (void)keypad_init();
  (void)ble_init();
  (void)storage_init();

  s_state = ST_BOOT;
  s_tick_sec = 0;
  record_reset();

  printf("\r\n==== %s ====\r\n", FW_VERSION_STR);
  printf("[boot] SYSCLK=%lu Hz\r\n", (unsigned long)SystemCoreClock);
  printf("[boot] I2C probe: OLED=%d MPU6050=%d MAX30102=%d\r\n",
         i2c_probe(I2C_ADDR_OLED), i2c_probe(I2C_ADDR_MPU6050),
         i2c_probe(I2C_ADDR_MAX30102));
}

void app_tick(void) {
  /* 心跳：LED 每 500ms 翻转，证明主循环存活 */
  static uint32_t last_blink;
  uint32_t now = HAL_GetTick();
  if (now - last_blink >= 500) {
    last_blink = now;
    LED_STATUS_TOGGLE();
    s_tick_sec = now / 1000;
  }

  switch (s_state) {
    case ST_BOOT:
      /* 自检完成后进入菜单（此处直接进入，键盘就绪后由 ui_flow 细化） */
      s_state = ST_MENU;
      break;
    case ST_MENU:
      /* TODO：键盘选择测量项；演示阶段直接跑一轮整机采集 */
      record_reset();
      measure_all();
      s_state = ST_RESULT;
      break;
    case ST_RESULT:
      printf("[result] score=%ld, 上报 BLE/存储\r\n",
             (long)g_current_record.score);
      (void)ble_send_record(&g_current_record);
      (void)storage_append(&g_current_record);
      oled_show_record(&g_current_record);
      s_state = ST_MENU;
      delay_ms(2000); /* 演示节流，正式版由 UI 事件驱动 */
      break;
    default:
      s_state = ST_MENU;
      break;
  }
}
