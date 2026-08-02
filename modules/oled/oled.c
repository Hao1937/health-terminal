/**
 * @file    oled.c
 * @owner   刘晏铭
 *
 * SSD1306 128x64 OLED（I2C 地址 0x3C，共用 SENSOR_I2C）。
 * 协议要点：每次 I2C 传输的第一字节是控制字节（Co=0 表示后续全是同类型字节），
 * 0x00 = 命令流、0x40 = 数据流。这恰好与 `i2c_mem_write(addr, reg, buf, len)`
 * 的 [addr][reg][buf...] 帧型一致，故直接把控制字节当作 `reg` 参数传入，
 * 不需要额外协议层。
 *
 * 寻址方式用 Page Addressing Mode（0x20,0x02）：每次先用 0xB0|page +
 * 列地址低/高 4 位两条命令定位光标，再连续写入若干字节（列自动递增）。
 *
 * 字体：自建 5x7 点阵，仅覆盖数字/大写字母/常用符号（. : - % /），
 * 小写字母显示时先转大写——足够菜单与体测结果这类纯 ASCII 短串使用，
 * 避免引入大字库占用 Flash。字模按「行」存储（每行 5bit，bit4=最左列），
 * 绘制时逐列转成 SSD1306 要求的「列字节」，避免手工转置引入的数据错误。
 */
#include "oled.h"

#if defined(MODULE_ENABLED_OLED)

#include <stdio.h>
#include <string.h>

#include "board.h"
#include "delay.h"
#include "i2c_bus.h"

#define OLED_ADDR I2C_ADDR_OLED
#define OLED_WIDTH 128u
#define OLED_HEIGHT 64u
#define OLED_PAGES 8u

#define FONT_W 5u
#define FONT_H 7u
#define OLED_TEXT_COLS (OLED_WIDTH / (FONT_W + 1u)) /* 每行最多字符数 = 21 */
#define MAX_TREND_SAMPLES 80u

/* ---- 5x7 字模（行存储，bit4=col0…bit0=col4），仅数字/大写字母/常用符号 ----
 */
static const uint8_t k_font_digit[10][FONT_H] = {
    {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}, /* 0 */
    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}, /* 1 */
    {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}, /* 2 */
    {0x0E, 0x11, 0x01, 0x06, 0x01, 0x11, 0x0E}, /* 3 */
    {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}, /* 4 */
    {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}, /* 5 */
    {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}, /* 6 */
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}, /* 7 */
    {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}, /* 8 */
    {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}, /* 9 */
};

static const uint8_t k_font_upper[26][FONT_H] = {
    {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, /* A */
    {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}, /* B */
    {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}, /* C */
    {0x1C, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1C}, /* D */
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}, /* E */
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}, /* F */
    {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E}, /* G */
    {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, /* H */
    {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}, /* I */
    {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C}, /* J */
    {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}, /* K */
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}, /* L */
    {0x11, 0x1B, 0x15, 0x11, 0x11, 0x11, 0x11}, /* M */
    {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}, /* N */
    {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, /* O */
    {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}, /* P */
    {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}, /* Q */
    {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}, /* R */
    {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}, /* S */
    {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}, /* T */
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, /* U */
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}, /* V */
    {0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11}, /* W */
    {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}, /* X */
    {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}, /* Y */
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}, /* Z */
};

static const uint8_t k_font_space[FONT_H] = {0x00, 0x00, 0x00, 0x00,
                                             0x00, 0x00, 0x00};
static const uint8_t k_font_dot[FONT_H] = {0x00, 0x00, 0x00, 0x00,
                                           0x00, 0x0C, 0x0C};
static const uint8_t k_font_colon[FONT_H] = {0x00, 0x0C, 0x0C, 0x00,
                                             0x0C, 0x0C, 0x00};
static const uint8_t k_font_dash[FONT_H] = {0x00, 0x00, 0x00, 0x1F,
                                            0x00, 0x00, 0x00};
static const uint8_t k_font_percent[FONT_H] = {0x11, 0x12, 0x02, 0x04,
                                               0x08, 0x09, 0x11};
static const uint8_t k_font_slash[FONT_H] = {0x01, 0x02, 0x02, 0x04,
                                             0x08, 0x08, 0x10};

static const uint8_t *font_lookup(char c) {
  if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
  if (c >= '0' && c <= '9') return k_font_digit[c - '0'];
  if (c >= 'A' && c <= 'Z') return k_font_upper[c - 'A'];
  switch (c) {
    case '.':
      return k_font_dot;
    case ':':
      return k_font_colon;
    case '-':
      return k_font_dash;
    case '%':
      return k_font_percent;
    case '/':
      return k_font_slash;
    default:
      return k_font_space; /* 未覆盖字符按空格处理，不阻塞显示 */
  }
}

/* ---- 底层命令/数据写入 ---- */
static int oled_write_cmd_buf(const uint8_t *cmds, uint16_t len) {
  return i2c_mem_write(OLED_ADDR, 0x00, cmds, len);
}

static int oled_write_data(const uint8_t *data, uint16_t len) {
  return i2c_mem_write(OLED_ADDR, 0x40, data, len);
}

/* 定位到 (x, page)：Page Addressing Mode 下列地址需拆成高/低 4 位两条命令 */
static int oled_set_cursor(uint8_t x, uint8_t page) {
  uint8_t cmds[3];
  cmds[0] = (uint8_t)(0xB0u | (page & 0x07u));
  cmds[1] = (uint8_t)(x & 0x0Fu);
  cmds[2] = (uint8_t)(0x10u | ((x >> 4) & 0x0Fu));
  return oled_write_cmd_buf(cmds, sizeof(cmds));
}

hs_status_t oled_init(void) {
  delay_ms(100); /* 上电稳定，SSD1306 手册建议的保守延时 */

  if (!i2c_probe(OLED_ADDR)) return HS_TIMEOUT;

  static const uint8_t k_init_cmds[] = {
      0xAE,       /* 显示关闭，配置期间不刷屏 */
      0xD5, 0x80, /* 显示时钟分频/振荡频率 */
      0xA8, 0x3F, /* 复用率 = 64 (0x3F) */
      0xD3, 0x00, /* 显示偏移 = 0 */
      0x40,       /* 显示起始行 = 0 */
      0x8D, 0x14, /* 使能内部电荷泵 */
      0x20, 0x02, /* 寻址模式 = Page Addressing Mode */
      0xA1,       /* 段重映射（列地址 127 -> SEG0） */
      0xC8,       /* COM 扫描方向重映射 */
      0xDA, 0x12, /* COM 引脚硬件配置 */
      0x81, 0xCF, /* 对比度 */
      0xD9, 0xF1, /* 预充电周期 */
      0xDB, 0x40, /* VCOMH 电平 */
      0xA4,       /* 恢复显示 GDDRAM 内容（非全亮/全暗） */
      0xA6,       /* 正常显示（非反色） */
      0xAF,       /* 显示开启 */
  };
  if (oled_write_cmd_buf(k_init_cmds, sizeof(k_init_cmds)) != 0) {
    return HS_TIMEOUT;
  }

  oled_clear();
  return HS_OK;
}

static const uint8_t k_zeros[OLED_WIDTH] = {0};

static void oled_clear_page(uint8_t page) {
  oled_set_cursor(0, page);
  oled_write_data(k_zeros, OLED_WIDTH);
}

void oled_clear(void) {
  for (uint8_t page = 0; page < OLED_PAGES; ++page) {
    oled_clear_page(page);
  }
}

/* 把行存储字模转成 5 个列字节（+1 列空白间距）后一次性写入 */
static void draw_char(uint8_t x, uint8_t page, char c) {
  const uint8_t *rows = font_lookup(c);
  uint8_t col_bytes[FONT_W + 1];

  for (uint8_t col = 0; col < FONT_W; ++col) {
    uint8_t v = 0;
    for (uint8_t row = 0; row < FONT_H; ++row) {
      if (rows[row] & (uint8_t)(1u << (FONT_W - 1u - col))) {
        v |= (uint8_t)(1u << row);
      }
    }
    col_bytes[col] = v;
  }
  col_bytes[FONT_W] = 0x00; /* 字符间 1px 空白 */

  oled_set_cursor(x, page);
  oled_write_data(col_bytes, sizeof(col_bytes));
}

void oled_show_text(uint8_t x, uint8_t page, const char *str) {
  if (str == NULL || page >= OLED_PAGES) return;

  uint8_t cx = x;
  for (const char *p = str; *p != '\0'; ++p) {
    if ((uint16_t)cx + (FONT_W + 1u) > OLED_WIDTH) break; /* 超出屏宽则截断 */
    draw_char(cx, page, *p);
    cx = (uint8_t)(cx + FONT_W + 1u);
  }
}

/* 反色版 draw_char：字形位取反，字符间距也填满（0xFF），否则反色条会断续 */
static void draw_char_inv(uint8_t x, uint8_t page, char c) {
  const uint8_t *rows = font_lookup(c);
  uint8_t col_bytes[FONT_W + 1];

  for (uint8_t col = 0; col < FONT_W; ++col) {
    uint8_t v = 0;
    for (uint8_t row = 0; row < FONT_H; ++row) {
      if (rows[row] & (uint8_t)(1u << (FONT_W - 1u - col))) {
        v |= (uint8_t)(1u << row);
      }
    }
    col_bytes[col] = (uint8_t)~v;
  }
  col_bytes[FONT_W] = 0xFF;

  oled_set_cursor(x, page);
  oled_write_data(col_bytes, sizeof(col_bytes));
}

void oled_show_text_inv(uint8_t x, uint8_t page, const char *str) {
  if (str == NULL || page >= OLED_PAGES) return;

  uint8_t cx = x;
  for (const char *p = str; *p != '\0'; ++p) {
    if ((uint16_t)cx + (FONT_W + 1u) > OLED_WIDTH) break;
    draw_char_inv(cx, page, *p);
    cx = (uint8_t)(cx + FONT_W + 1u);
  }
  /* 行尾剩余宽度也填满反色背景，凑成一整条高亮条（不止字符本身宽度） */
  if (cx < OLED_WIDTH) {
    uint8_t fill[OLED_WIDTH];
    uint8_t remain = (uint8_t)(OLED_WIDTH - cx);
    memset(fill, 0xFF, remain);
    oled_set_cursor(cx, page);
    oled_write_data(fill, remain);
  }
}

/* ---- 定点数格式化（不用软浮点，遵循整机"全程定点整数"约定） ---- */
static void fmt_dec1(int32_t raw, int32_t divisor, char *buf, size_t n) {
  if (raw == HS_VALUE_INVALID) {
    snprintf(buf, n, "--");
    return;
  }
  int32_t whole = raw / divisor;
  int32_t frac = (raw % divisor) * 10 / divisor;
  if (frac < 0) frac = -frac;
  snprintf(buf, n, "%ld.%ld", (long)whole, (long)frac);
}

static void fmt_dec2(int32_t raw, char *buf, size_t n) {
  if (raw == HS_VALUE_INVALID) {
    snprintf(buf, n, "--");
    return;
  }
  int32_t whole = raw / 100;
  int32_t frac = raw % 100;
  if (frac < 0) frac = -frac;
  snprintf(buf, n, "%ld.%02ld", (long)whole, (long)frac);
}

static void fmt_int(int32_t raw, char *buf, size_t n) {
  if (raw == HS_VALUE_INVALID) {
    snprintf(buf, n, "--");
    return;
  }
  snprintf(buf, n, "%ld", (long)raw);
}

void oled_show_record(const measurement_record_t *rec) {
  if (rec == NULL) return;

  char a[16], b[16], tmp[48], line[48];

  /* 定宽（补空格）后原地覆盖旧内容，不做整屏清空，避免每次刷新都黑屏闪烁 */
  fmt_dec1(rec->height_mm, 10, a, sizeof(a));  /* mm -> cm，1 位小数 */
  fmt_dec1(rec->weight_g, 1000, b, sizeof(b)); /* g  -> kg，1 位小数 */
  snprintf(tmp, sizeof(tmp), "H:%s W:%s", a, b);
  snprintf(line, sizeof(line), "%-*s", (int)OLED_TEXT_COLS, tmp);
  oled_show_text(0, 0, line);

  fmt_dec2(rec->bmi_x100, a, sizeof(a));
  fmt_dec1(rec->bodyfat_x10, 10, b, sizeof(b));
  snprintf(tmp, sizeof(tmp), "BMI:%s FAT:%s", a, b);
  snprintf(line, sizeof(line), "%-*s", (int)OLED_TEXT_COLS, tmp);
  oled_show_text(0, 1, line);

  fmt_int(rec->heart_rate_bpm, a, sizeof(a));
  fmt_dec1(rec->spo2_x10, 10, b, sizeof(b));
  snprintf(tmp, sizeof(tmp), "HR:%s SPO2:%s", a, b);
  snprintf(line, sizeof(line), "%-*s", (int)OLED_TEXT_COLS, tmp);
  oled_show_text(0, 2, line);

  fmt_dec1(rec->balance_x10, 10, a, sizeof(a));
  fmt_dec1(rec->grip_kg_x10, 10, b, sizeof(b));
  snprintf(tmp, sizeof(tmp), "BAL:%s GRIP:%s", a, b);
  snprintf(line, sizeof(line), "%-*s", (int)OLED_TEXT_COLS, tmp);
  oled_show_text(0, 3, line);

  fmt_int(rec->reaction_ms, a, sizeof(a));
  snprintf(tmp, sizeof(tmp), "RT:%sms", a);
  snprintf(line, sizeof(line), "%-*s", (int)OLED_TEXT_COLS, tmp);
  oled_show_text(0, 4, line);

  fmt_int(rec->score, a, sizeof(a));
  snprintf(tmp, sizeof(tmp), "SCORE:%s", a);
  snprintf(line, sizeof(line), "%-*s", (int)OLED_TEXT_COLS, tmp);
  oled_show_text(0, 5, line);

  /* 6 行用不到 page 6/7：清一次即可，之后一直为空不会再产生视觉闪烁 */
  oled_clear_page(6);
  oled_clear_page(7);
}

static void trend_set_pixel(uint8_t canvas[OLED_PAGES][OLED_WIDTH], uint8_t x,
                            uint8_t y) {
  if (x >= OLED_WIDTH || y >= OLED_HEIGHT) return;
  canvas[y / 8u][x] |= (uint8_t)(1u << (y & 0x07u));
}

static void trend_draw_line(uint8_t canvas[OLED_PAGES][OLED_WIDTH], int x0,
                            int y0, int x1, int y1) {
  int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
  int sx = (x0 < x1) ? 1 : -1;
  int dy = (y1 > y0) ? (y0 - y1) : (y1 - y0);
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx + dy;

  while (1) {
    trend_set_pixel(canvas, (uint8_t)x0, (uint8_t)y0);
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

void oled_show_trend(const int32_t *series, uint16_t n) {
  static uint8_t canvas[OLED_PAGES][OLED_WIDTH];
  uint8_t xs[MAX_TREND_SAMPLES];
  uint8_t ys[MAX_TREND_SAMPLES];

  if (series == NULL || n == 0) {
    oled_clear();
    return;
  }
  if (n > MAX_TREND_SAMPLES) n = MAX_TREND_SAMPLES;

  memset(canvas, 0, sizeof(canvas));

  int32_t vmin = series[0];
  int32_t vmax = series[0];
  for (uint16_t i = 1; i < n; ++i) {
    if (series[i] < vmin) vmin = series[i];
    if (series[i] > vmax) vmax = series[i];
  }
  int32_t range = vmax - vmin;

  for (uint8_t x = 0; x < OLED_WIDTH; ++x) trend_set_pixel(canvas, x, 63);
  for (uint8_t y = 0; y < OLED_HEIGHT; ++y) trend_set_pixel(canvas, 0, y);

  for (uint16_t i = 0; i < n; ++i) {
    xs[i] = (n == 1u) ? (OLED_WIDTH / 2u)
                      : (uint8_t)(4u + ((uint32_t)i * (OLED_WIDTH - 8u)) /
                                           (uint32_t)(n - 1u));
    if (range == 0) {
      ys[i] = OLED_HEIGHT / 2u;
    } else {
      uint32_t scaled =
          (uint32_t)(((int64_t)(series[i] - vmin) * (OLED_HEIGHT - 12u)) /
                     range);
      ys[i] = (uint8_t)((OLED_HEIGHT - 6u) - scaled);
    }
  }

  for (uint16_t i = 1; i < n; ++i) {
    trend_draw_line(canvas, xs[i - 1u], ys[i - 1u], xs[i], ys[i]);
  }
  for (uint16_t i = 0; i < n; ++i) {
    trend_set_pixel(canvas, xs[i], ys[i]);
    if (xs[i] > 0) trend_set_pixel(canvas, (uint8_t)(xs[i] - 1u), ys[i]);
    if (xs[i] + 1u < OLED_WIDTH)
      trend_set_pixel(canvas, (uint8_t)(xs[i] + 1u), ys[i]);
    if (ys[i] > 0) trend_set_pixel(canvas, xs[i], (uint8_t)(ys[i] - 1u));
    if (ys[i] + 1u < OLED_HEIGHT)
      trend_set_pixel(canvas, xs[i], (uint8_t)(ys[i] + 1u));
  }

  for (uint8_t page = 0; page < OLED_PAGES; ++page) {
    oled_set_cursor(0, page);
    oled_write_data(canvas[page], OLED_WIDTH);
  }
}

#else /* 本板 MODULE_SET 未包含该模块：桩替代，仅供 app 注册表链接 */

hs_status_t oled_init(void) { return HS_NOT_IMPLEMENTED; }
void oled_clear(void) {}
void oled_show_text(uint8_t x, uint8_t page, const char *str) {
  (void)x;
  (void)page;
  (void)str;
}
void oled_show_text_inv(uint8_t x, uint8_t page, const char *str) {
  (void)x;
  (void)page;
  (void)str;
}
void oled_show_record(const measurement_record_t *rec) { (void)rec; }
void oled_show_trend(const int32_t *series, uint16_t n) {
  (void)series;
  (void)n;
}

#endif
