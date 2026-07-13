/**
 * @file    oled.h
 * @brief   SSD1306 OLED 菜单与趋势曲线（I2C 0x3C）。
 * @owner   刘晏铭
 */
#ifndef MODULE_OLED_H
#define MODULE_OLED_H

#include "health_if.h"
#include "health_record.h"

hs_status_t oled_init(void);
void oled_clear(void);
void oled_show_text(uint8_t x, uint8_t page, const char *str);
/** @brief 在屏上展示一条体测记录（8 项指标+评分分页显示）。 */
void oled_show_record(const measurement_record_t *rec);
/** @brief 绘制某指标的历史趋势曲线。 */
void oled_show_trend(const int32_t *series, uint16_t n);

#endif /* MODULE_OLED_H */
