/**
 * @file    keypad.c
 * @owner   刘晏铭
 *
 * 4x4 矩阵键盘：逐行拉低扫列。行 PB12-15 推挽输出（空闲全拉高），
 * 列 PA4-7 上拉输入（被扫描行拉低时按下的列会读到低电平）。
 *
 *   Row0(PB12): '1' '2' '3' 'A'
 *   Row1(PB13): '4' '5' '6' 'B'
 *   Row2(PB14): '7' '8' '9' 'C'
 *   Row3(PB15): '*' '0' '#' 'D'
 *
 * keypad_scan() 由主循环每 tick 调用（无节流），必须非阻塞；用 tick_now_ms()
 * 做时间去抖 + 边沿触发，保证一次物理按下只报一次，长按不重复。
 */
#include "keypad.h"

#if defined(MODULE_ENABLED_KEYPAD)

#include "board.h"
#include "delay.h"
#include "tick.h"

#define KEYPAD_DEBOUNCE_MS 15u
#define ROWS_ALL \
  (KEYPAD_ROW0_PIN | KEYPAD_ROW1_PIN | KEYPAD_ROW2_PIN | KEYPAD_ROW3_PIN)
#define COLS_ALL \
  (KEYPAD_COL0_PIN | KEYPAD_COL1_PIN | KEYPAD_COL2_PIN | KEYPAD_COL3_PIN)

static const uint16_t k_row_pins[4] = {KEYPAD_ROW0_PIN, KEYPAD_ROW1_PIN,
                                       KEYPAD_ROW2_PIN, KEYPAD_ROW3_PIN};
static const uint16_t k_col_pins[4] = {KEYPAD_COL0_PIN, KEYPAD_COL1_PIN,
                                       KEYPAD_COL2_PIN, KEYPAD_COL3_PIN};
static const char k_keymap[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'},
};

/* 去抖状态机：raw 每次扫描的原始结果；candidate 稳定计时中的候选值；
 * stable 是已确认的按下键；reported 保证 stable 只被上报一次（边沿触发）。 */
static char s_stable;
static char s_candidate;
static char s_reported;
static uint32_t s_candidate_since;

/* 一次调用内完整扫完 4 行，返回命中的键值（无按下返回 0） */
static char raw_scan_matrix(void) {
  char hit = 0;
  for (uint8_t r = 0; r < 4 && hit == 0; ++r) {
    HAL_GPIO_WritePin(KEYPAD_ROW_PORT, ROWS_ALL, GPIO_PIN_SET);
    HAL_GPIO_WritePin(KEYPAD_ROW_PORT, k_row_pins[r], GPIO_PIN_RESET);
    delay_us(2); /* 行电平建立 */
    for (uint8_t c = 0; c < 4; ++c) {
      if (HAL_GPIO_ReadPin(KEYPAD_COL_PORT, k_col_pins[c]) == GPIO_PIN_RESET) {
        hit = k_keymap[r][c];
        break;
      }
    }
  }
  HAL_GPIO_WritePin(KEYPAD_ROW_PORT, ROWS_ALL, GPIO_PIN_SET); /* 恢复空闲态 */
  return hit;
}

hs_status_t keypad_init(void) {
  GPIO_InitTypeDef g = {0};

  g.Pin = ROWS_ALL;
  g.Mode = GPIO_MODE_OUTPUT_PP;
  g.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(KEYPAD_ROW_PORT, &g);
  HAL_GPIO_WritePin(KEYPAD_ROW_PORT, ROWS_ALL, GPIO_PIN_SET);

  g.Pin = COLS_ALL;
  g.Mode = GPIO_MODE_INPUT;
  g.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(KEYPAD_COL_PORT, &g);

  s_stable = 0;
  s_candidate = 0;
  s_reported = 0;
  s_candidate_since = tick_now_ms();
  return HS_OK;
}

char keypad_scan(void) {
  char raw = raw_scan_matrix();
  uint32_t now = tick_now_ms();

  if (raw != s_candidate) {
    s_candidate = raw;
    s_candidate_since = now;
  }
  if ((now - s_candidate_since) >= KEYPAD_DEBOUNCE_MS &&
      s_candidate != s_stable) {
    s_stable = s_candidate;
    s_reported = 0;
  }

  if (s_stable != 0 && !s_reported) {
    s_reported = 1;
    return s_stable;
  }
  return 0;
}

#else
hs_status_t keypad_init(void) { return HS_NOT_IMPLEMENTED; }
char keypad_scan(void) { return 0; }
#endif
