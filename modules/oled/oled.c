/**
 * @file    oled.c
 * @owner   刘晏铭
 * TODO(刘晏铭)：SSD1306 初始化序列 + 字库 + 页写入；趋势曲线用列写点。
 */
#include "oled.h"

#if defined(MODULE_ENABLED_OLED)
hs_status_t oled_init(void) { return HS_NOT_IMPLEMENTED; }
void oled_clear(void) { }
void oled_show_text(uint8_t x, uint8_t page, const char *str) { (void)x;(void)page;(void)str; }
void oled_show_record(const measurement_record_t *rec) { (void)rec; }
void oled_show_trend(const int32_t *series, uint16_t n) { (void)series;(void)n; }
#else
hs_status_t oled_init(void) { return HS_NOT_IMPLEMENTED; }
void oled_clear(void) { }
void oled_show_text(uint8_t x, uint8_t page, const char *str) { (void)x;(void)page;(void)str; }
void oled_show_record(const measurement_record_t *rec) { (void)rec; }
void oled_show_trend(const int32_t *series, uint16_t n) { (void)series;(void)n; }
#endif
