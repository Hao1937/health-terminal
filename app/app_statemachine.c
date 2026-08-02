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
#include "ui_flow.h"

static void current_record_prepare_partial(void);

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

static void current_record_prepare_partial(void) {
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

static void record_reset(void) { current_record_prepare_partial(); }

/* 采集单个测量项（供 ui_flow 按项触发，而不是每次都跑全表） */
static void measure_registry_item(hs_item_t item) {
  for (const hs_sensor_t *s = g_hs_registry; s->measure != NULL; ++s) {
    if (s->item != item) continue;
    hs_status_t init_st = s->init ? s->init() : HS_OK;
    if (init_st != HS_OK) {
      printf("[measure] %-9s (%s) init -> %s\r\n", s->name, s->owner,
             hs_status_str(init_st));
      return;
    }
    hs_sample_t sample = {HS_VALUE_INVALID, HS_VALUE_INVALID};
    hs_status_t st = s->measure(&sample);
    printf("[measure] %-9s (%s) -> %s", s->name, s->owner, hs_status_str(st));
    if (st == HS_OK) {
      printf(" primary=%ld secondary=%ld", (long)sample.primary,
             (long)sample.secondary);
    }
    printf("\r\n");
    if (st == HS_OK) store_sample(s->item, &sample);
    return;
  }
}

void app_init(void) {
  board_gpio_init();
  delay_init();
  uart_init();
  setvbuf(stdout, NULL, _IONBF, 0);
  printf("\r\n[boot] uart ready\r\n");
  i2c_bus_init();
  hs_status_t oled_st = oled_init();
  hs_status_t keypad_st = keypad_init();
  (void)ble_init();
  (void)storage_init();

  s_state = ST_BOOT;
  s_tick_sec = 0;
  record_reset();

  printf("\r\n==== %s ====\r\n", FW_VERSION_STR);
  printf("[boot] SYSCLK=%lu Hz\r\n", (unsigned long)SystemCoreClock);
  printf("[boot] OLED=%s KEYPAD=%s\r\n", hs_status_str(oled_st),
         hs_status_str(keypad_st));
  printf("[boot] I2C probe: OLED=%d MPU6050=%d MAX30102=%d\r\n",
         i2c_probe(I2C_ADDR_OLED), i2c_probe(I2C_ADDR_MPU6050),
         i2c_probe(I2C_ADDR_MAX30102));

  if (oled_st == HS_OK) {
    oled_clear();
    oled_show_text(0, 0, FW_VERSION_STR);
    oled_show_text(0, 2, "OLED:OK");
    oled_show_text(0, 3, (keypad_st == HS_OK) ? "KEYPAD:OK" : "KEYPAD:ERR");
    oled_show_text(0, 5, "READY");
  }
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
    case ST_MENU: {
      ui_tick_result_t r = ui_tick();
      if (r.action == UI_ACT_MEASURE) {
        oled_show_text(0, 6, "MEASURING       ");
        for (uint8_t i = 0; i < r.item_count; ++i) {
          measure_registry_item(r.items[i]);
        }
        if (g_current_record.height_mm != HS_VALUE_INVALID &&
            g_current_record.weight_g != HS_VALUE_INVALID) {
          bmi_compute(g_current_record.height_mm, g_current_record.weight_g,
                      &g_current_record.bmi_x100);
        }
        ui_request_redraw();
      } else if (r.action == UI_ACT_UPLOAD) {
        oled_show_text(0, 6, "UPLOADING       ");
        int32_t score;
        if (health_score_compute(&g_current_record, &score) == HS_OK) {
          g_current_record.score = score;
        }
        record_finalize(&g_current_record);
        s_state = ST_RESULT;
      }
      break;
    }
    case ST_RESULT: {
      /* 副作用只能在“刚进入本状态”那一次执行；等按键期间每 tick 都会
       * 再跑一次 app_tick()，若不加这个门槛 BLE/Flash 会被反复触发。 */
      static int s_shown = 0;
      if (!s_shown) {
        printf("[result] score=%ld, 上报 BLE/存储\r\n",
               (long)g_current_record.score);
        (void)ble_send_record(&g_current_record);
        (void)storage_append(&g_current_record);
        oled_show_record(&g_current_record);
        s_shown = 1;
      }
      if (keypad_scan() != 0) {
        s_shown = 0;
        record_reset();
        ui_reset_to_root();
        s_state = ST_MENU;
      }
      break;
    }
    default:
      s_state = ST_MENU;
      break;
  }
}
