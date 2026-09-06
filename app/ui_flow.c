/**
 * @file    ui_flow.c
 * @owner   刘晏铭
 *
 * 多级菜单：根菜单(7项) -> 每项操作子菜单(Start/Back) -> History 再深入到
 * 记录列表 -> 记录详情 -> 单指标历史趋势图。按键语义：
 *   数字 1-7  任意页面直接一步跳入对应项（不需要先返回根菜单）
 *   A         确认/进入
 *   B         返回上一级
 *   C / D     上一项 / 下一项（列表、子菜单选项、指标切换通用）
 *
 * 只有「读到按键」或「刚进入新画面」时才重绘 OLED（走 I2C 有开销），避免每个
 * 空转 tick 都刷屏。History 相关画面依赖 modules/storage 提供的 Flash 历史。
 */
#include "ui_flow.h"

#include <stddef.h>
#include <stdio.h>

#include "app.h"
#include "keypad.h"
#include "oled.h"
#include "storage.h"

/* ---- 根菜单：文案 + 对应的传感器项 ---- */
typedef struct {
  const char *label;
  hs_item_t items[2];
  uint8_t item_count; /* 0 表示特殊项（is_upload 或 is_history） */
  uint8_t is_upload;  /* 第 6 项：Score/Upload */
  uint8_t is_history; /* 第 7 项：History */
} root_entry_t;

static const root_entry_t k_root[7] = {
    {"1.Height/Weight", {HS_ITEM_HEIGHT, HS_ITEM_WEIGHT}, 2, 0, 0},
    {"2.HR/SpO2", {HS_ITEM_HR_SPO2, HS_ITEM_HR_SPO2}, 1, 0, 0},
    {"3.Balance", {HS_ITEM_BALANCE, HS_ITEM_BALANCE}, 1, 0, 0},
    {"4.Grip", {HS_ITEM_GRIP, HS_ITEM_GRIP}, 1, 0, 0},
    {"5.Reaction", {HS_ITEM_REACTION, HS_ITEM_REACTION}, 1, 0, 0},
    {"6.Score/Upload", {HS_ITEM_HEIGHT, HS_ITEM_HEIGHT}, 0, 1, 0},
    {"7.History", {HS_ITEM_HEIGHT, HS_ITEM_HEIGHT}, 0, 0, 1},
};
#define ROOT_COUNT 7u

/* ---- 单条记录里可查看趋势的指标（offsetof 表，免十几个 case） ---- */
typedef struct {
  const char *label;
  size_t off;
} trend_metric_t;

static const trend_metric_t k_metrics[] = {
    {"HEIGHT", offsetof(measurement_record_t, height_mm)},
    {"WEIGHT", offsetof(measurement_record_t, weight_g)},
    {"BMI", offsetof(measurement_record_t, bmi_x100)},
    {"BODYFAT", offsetof(measurement_record_t, bodyfat_x10)},
    {"HR", offsetof(measurement_record_t, heart_rate_bpm)},
    {"SPO2", offsetof(measurement_record_t, spo2_x10)},
    {"BALANCE", offsetof(measurement_record_t, balance_x10)},
    {"GRIP", offsetof(measurement_record_t, grip_kg_x10)},
    {"REACTION", offsetof(measurement_record_t, reaction_ms)},
    {"SCORE", offsetof(measurement_record_t, score)},
};
#define METRIC_COUNT (sizeof(k_metrics) / sizeof(k_metrics[0]))

#define HISTORY_ROWS 6u  /* 列表一屏最多显示的记录行数（page1-6） */
#define UI_TEXT_COLS 21u /* 128px / (5px 字宽 + 1px 间距) */
#define MAX_TREND_SAMPLES 80u
static int32_t s_trend_buf[MAX_TREND_SAMPLES];

typedef enum {
  UI_SCR_ROOT_MENU = 0,
  UI_SCR_SUBMENU,
  UI_SCR_HISTORY_LIST,
  UI_SCR_HISTORY_DETAIL,
  UI_SCR_HISTORY_TREND,
} ui_screen_t;

static ui_screen_t s_screen = UI_SCR_ROOT_MENU;
static uint8_t s_root_sel;
static uint8_t s_submenu_sel; /* 0=Start 1=Back */
static uint16_t s_hist_sel;   /* 选中的历史记录（0=最旧, count-1=最新） */
static uint16_t s_hist_top;   /* 列表滚动窗口起始下标 */
static uint8_t s_metric_sel;  /* 详情页当前循环到的指标 */
static measurement_record_t s_hist_rec;
static char s_last_key = '-';
static uint8_t s_needs_redraw = 1;

/* 跳过 HS_VALUE_INVALID / 读取失败的槽位，避免一个未测过的历史指标把
 * oled_show_trend 的 min/max 归一化拉爆（图表变成一根尖刺）。 */
static uint16_t gather_series(size_t off, int32_t *out, uint16_t cap) {
  uint16_t total = storage_count();
  uint16_t n = (total < cap) ? total : cap;
  uint16_t start = (uint16_t)(total - n);
  uint16_t w = 0;
  for (uint16_t i = 0; i < n; ++i) {
    measurement_record_t rec;
    if (storage_read((uint16_t)(start + i), &rec) != HS_OK) continue;
    int32_t v = *(const int32_t *)((const uint8_t *)&rec + off);
    if (v == HS_VALUE_INVALID) continue;
    out[w++] = v;
  }
  return w;
}

static void clamp_hist_window(uint16_t count) {
  if (count == 0) {
    s_hist_sel = 0;
    s_hist_top = 0;
    return;
  }
  if (s_hist_sel >= count) s_hist_sel = (uint16_t)(count - 1);
  if (s_hist_sel < s_hist_top) s_hist_top = s_hist_sel;
  if (s_hist_sel >= s_hist_top + HISTORY_ROWS) {
    s_hist_top = (uint16_t)(s_hist_sel - HISTORY_ROWS + 1);
  }
}

/* ---- 按键分派 ---- */
static void handle_root(char key, ui_tick_result_t *r) {
  (void)r;
  if (key >= '1' && key <= '7') {
    s_root_sel = (uint8_t)(key - '1');
    if (k_root[s_root_sel].is_history) {
      s_screen = UI_SCR_HISTORY_LIST;
      s_hist_sel = 0;
      s_hist_top = 0;
      clamp_hist_window(storage_count());
    } else {
      s_screen = UI_SCR_SUBMENU;
      s_submenu_sel = 0;
    }
    return;
  }
  switch (key) {
    case 'C':
      s_root_sel = (uint8_t)((s_root_sel + ROOT_COUNT - 1) % ROOT_COUNT);
      break;
    case 'D':
      s_root_sel = (uint8_t)((s_root_sel + 1) % ROOT_COUNT);
      break;
    case 'A':
      if (k_root[s_root_sel].is_history) {
        s_screen = UI_SCR_HISTORY_LIST;
        s_hist_sel = 0;
        s_hist_top = 0;
        clamp_hist_window(storage_count());
      } else {
        s_screen = UI_SCR_SUBMENU;
        s_submenu_sel = 0;
      }
      break;
    default:
      break;
  }
}

static void handle_submenu(char key, ui_tick_result_t *r) {
  const root_entry_t *e = &k_root[s_root_sel];
  switch (key) {
    case 'C':
    case 'D':
      s_submenu_sel = (uint8_t)(s_submenu_sel ^ 1u);
      break;
    case 'B':
      s_screen = UI_SCR_ROOT_MENU;
      break;
    case 'A':
      if (s_submenu_sel == 1) {
        s_screen = UI_SCR_ROOT_MENU;
      } else if (e->is_upload) {
        r->action = UI_ACT_UPLOAD;
      } else {
        r->action = UI_ACT_MEASURE;
        r->item_count = e->item_count;
        r->items[0] = e->items[0];
        r->items[1] = e->items[1];
      }
      break;
    default:
      break;
  }
}

static void handle_history_list(char key, ui_tick_result_t *r) {
  (void)r;
  uint16_t count = storage_count();
  if (key == 'B') {
    s_screen = UI_SCR_ROOT_MENU;
    return;
  }
  if (count == 0) return;
  switch (key) {
    case 'C':
      if (s_hist_sel > 0) --s_hist_sel;
      clamp_hist_window(count);
      break;
    case 'D':
      if (s_hist_sel + 1 < count) ++s_hist_sel;
      clamp_hist_window(count);
      break;
    case 'A':
      if (storage_read((uint16_t)(count - 1 - s_hist_sel), &s_hist_rec) ==
          HS_OK) {
        s_screen = UI_SCR_HISTORY_DETAIL;
        s_metric_sel = 0;
      }
      break;
    default:
      break;
  }
}

static void handle_history_detail(char key, ui_tick_result_t *r) {
  (void)r;
  switch (key) {
    case 'C':
      s_metric_sel =
          (uint8_t)((s_metric_sel + METRIC_COUNT - 1) % METRIC_COUNT);
      break;
    case 'D':
      s_metric_sel = (uint8_t)((s_metric_sel + 1) % METRIC_COUNT);
      break;
    case 'B':
      s_screen = UI_SCR_HISTORY_LIST;
      break;
    case 'A':
      s_screen = UI_SCR_HISTORY_TREND;
      break;
    default:
      break;
  }
}

static void handle_history_trend(char key, ui_tick_result_t *r) {
  (void)r;
  if (key == 'B') s_screen = UI_SCR_HISTORY_DETAIL;
}

static void handle_key(char key, ui_tick_result_t *r) {
  if (key >= '1' && key <= '7' && s_screen != UI_SCR_ROOT_MENU) {
    s_root_sel = (uint8_t)(key - '1');
    if (k_root[s_root_sel].is_history) {
      s_screen = UI_SCR_HISTORY_LIST;
      s_hist_sel = 0;
      s_hist_top = 0;
      clamp_hist_window(storage_count());
    } else {
      s_screen = UI_SCR_SUBMENU;
      s_submenu_sel = 0;
    }
    return;
  }

  switch (s_screen) {
    case UI_SCR_ROOT_MENU:
      handle_root(key, r);
      break;
    case UI_SCR_SUBMENU:
      handle_submenu(key, r);
      break;
    case UI_SCR_HISTORY_LIST:
      handle_history_list(key, r);
      break;
    case UI_SCR_HISTORY_DETAIL:
      handle_history_detail(key, r);
      break;
    case UI_SCR_HISTORY_TREND:
      handle_history_trend(key, r);
      break;
    default:
      break;
  }
}

/* ---- 渲染 ---- */
static void show_text_line(uint8_t page, const char *str) {
  char line[24];
  snprintf(line, sizeof(line), "%-*s", (int)UI_TEXT_COLS, str);
  oled_show_text(0, page, line);
}

static void render_root_menu(void) {
  for (uint8_t i = 0; i < ROOT_COUNT; ++i) {
    if (i == s_root_sel) {
      oled_show_text_inv(0, i, k_root[i].label);
    } else {
      show_text_line(i, k_root[i].label);
    }
  }
}

static void render_submenu(void) {
  const root_entry_t *e = &k_root[s_root_sel];
  char line[48];

  show_text_line(0, e->label);

  if (e->is_upload) {
    snprintf(line, sizeof(line), "SCORE:%s",
             (g_current_record.score != HS_VALUE_INVALID) ? "OK" : "--");
  } else if (e->item_count == 2) {
    char height[20];
    char weight[20];
    if (g_current_record.height_mm == HS_VALUE_INVALID) {
      snprintf(height, sizeof(height), "--");
    } else {
      /* mm -> cm，保留一位小数：1732mm 显示为 173.2CM。 */
      snprintf(height, sizeof(height), "%ld.%ldCM",
               (long)(g_current_record.height_mm / 10),
               (long)(g_current_record.height_mm % 10));
    }
    if (g_current_record.weight_g == HS_VALUE_INVALID) {
      snprintf(weight, sizeof(weight), "--");
    } else {
      /* g -> kg，保留一位小数：65300g 显示为 65.3KG。 */
      snprintf(weight, sizeof(weight), "%ld.%ldKG",
               (long)(g_current_record.weight_g / 1000),
               (long)((g_current_record.weight_g % 1000) / 100));
    }
    snprintf(line, sizeof(line), "H:%s W:%s", height, weight);
  } else {
    const char *ok = "--";
    switch (e->items[0]) {
      case HS_ITEM_HR_SPO2:
        if (g_current_record.heart_rate_bpm != HS_VALUE_INVALID &&
            g_current_record.spo2_x10 != HS_VALUE_INVALID) {
          /* 血氧以 ×10 定点保存：976 显示为 97.6%。 */
          snprintf(line, sizeof(line), "HR:%ld SPO2:%ld.%ld%%",
                   (long)g_current_record.heart_rate_bpm,
                   (long)(g_current_record.spo2_x10 / 10),
                   (long)(g_current_record.spo2_x10 % 10));
        } else {
          snprintf(line, sizeof(line), "HR:-- SPO2:--");
        }
        ok = NULL;
        break;
      case HS_ITEM_BALANCE:
        if (g_current_record.balance_x10 != HS_VALUE_INVALID) {
          snprintf(line, sizeof(line), "BAL:%ld",
                   (long)g_current_record.balance_x10);
        } else {
          snprintf(line, sizeof(line), "BAL:--");
        }
        ok = NULL;
        break;
      case HS_ITEM_GRIP:
        ok = (g_current_record.grip_kg_x10 != HS_VALUE_INVALID) ? "OK" : "--";
        break;
      case HS_ITEM_REACTION:
        if (g_current_record.reaction_ms != HS_VALUE_INVALID) {
          snprintf(line, sizeof(line), "RT:%ldMS",
                   (long)g_current_record.reaction_ms);
        } else {
          snprintf(line, sizeof(line), "RT:--MS");
        }
        ok = NULL;
        break;
      default:
        break;
    }
    if (ok != NULL) snprintf(line, sizeof(line), "STATUS:%s", ok);
  }
  show_text_line(1, line);

  if (s_submenu_sel == 0) {
    oled_show_text_inv(0, 3, "Start");
    show_text_line(4, "Back");
  } else {
    show_text_line(3, "Start");
    oled_show_text_inv(0, 4, "Back");
  }
  show_text_line(2, "");
  show_text_line(5, "A:OK B:BACK C/D:NAV");
  show_text_line(6, "");
}

static void render_history_list(void) {
  uint16_t count = storage_count();
  char line[40];

  snprintf(line, sizeof(line), "HIST %u/%u",
           count ? (unsigned)(s_hist_sel + 1) : 0u, (unsigned)count);
  show_text_line(0, line);

  if (count == 0) {
    show_text_line(3, "NO RECORDS");
    return;
  }

  for (uint16_t row = 0; row < HISTORY_ROWS; ++row) {
    uint16_t sel_idx = (uint16_t)(s_hist_top + row);
    if (sel_idx >= count) break;
    measurement_record_t rec;
    char score_str[16];
    if (storage_read((uint16_t)(count - 1 - sel_idx), &rec) == HS_OK &&
        rec.score != HS_VALUE_INVALID) {
      snprintf(score_str, sizeof(score_str), "%ld", (long)rec.score);
    } else {
      snprintf(score_str, sizeof(score_str), "--");
    }
    snprintf(line, sizeof(line), "#%-3u SCORE:%s", (unsigned)(sel_idx + 1),
             score_str);
    if (sel_idx == s_hist_sel) {
      oled_show_text_inv(0, (uint8_t)(1 + row), line);
    } else {
      show_text_line((uint8_t)(1 + row), line);
    }
  }
}

static void render_history_detail(void) {
  oled_show_record(&s_hist_rec);
  char line[24];
  snprintf(line, sizeof(line), "METRIC:%s", k_metrics[s_metric_sel].label);
  oled_show_text(0, 6, line);
}

static void render_history_trend(void) {
  uint16_t n = gather_series(k_metrics[s_metric_sel].off, s_trend_buf,
                             MAX_TREND_SAMPLES);
  if (n == 0) {
    oled_clear();
    oled_show_text(0, 3, "NO DATA");
  } else {
    oled_show_trend(s_trend_buf, n);
  }
}

static void render_footer(void) {
  char buf[8];
  snprintf(buf, sizeof(buf), "KEY:%c", s_last_key);
  oled_show_text(98, 7, buf);
}

static void render_current_screen(void) {
  switch (s_screen) {
    case UI_SCR_ROOT_MENU:
      render_root_menu();
      break;
    case UI_SCR_SUBMENU:
      render_submenu();
      break;
    case UI_SCR_HISTORY_LIST:
      render_history_list();
      break;
    case UI_SCR_HISTORY_DETAIL:
      render_history_detail();
      break;
    case UI_SCR_HISTORY_TREND:
      render_history_trend();
      break;
    default:
      break;
  }
}

ui_tick_result_t ui_tick(void) {
  ui_tick_result_t r = {UI_ACT_NONE, {0, 0}, 0};
  static int s_prev_screen = -1; /* 用 int 装哨兵值，区别于任何合法枚举值 */

  char key = keypad_scan();
  if (key != 0) {
    s_last_key = key;
    s_needs_redraw = 1;
    handle_key(key, &r);
  }

  if (s_needs_redraw) {
    if ((int)s_screen != s_prev_screen) {
      /* 切换画面：不同画面用到的页不同，先清屏避免旧画面残留像素
       * （例如根菜单用 0-6 页，子菜单只画 0/1/3/4 页，若不清屏 2/5/6
       * 页会保留上一屏的文字，和新内容叠在一起看着像乱码）。 */
      oled_clear();
      s_prev_screen = (int)s_screen;
    }
    render_current_screen();
    if (s_screen != UI_SCR_HISTORY_TREND) render_footer();
    s_needs_redraw = 0;
  }

  return r;
}

void ui_reset_to_root(void) {
  s_screen = UI_SCR_ROOT_MENU;
  s_root_sel = 0;
  s_needs_redraw = 1;
}

void ui_request_redraw(void) { s_needs_redraw = 1; }
