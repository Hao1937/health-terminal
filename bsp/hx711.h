/**
 * @file    hx711.h
 * @brief   HX711 24 位称重 ADC 的可复用底层驱动（软件时序，与具体秤体无关）。
 * @owner   王宇浩（组长）；使用者：握力(hx711_grip) 与 体重(hx711_weight, 查樊听)
 *
 * HX711 是两线接口（SCK 输出 + DOUT 输入），无标准总线，靠 MCU 打拍读 24bit。
 * 本驱动做成「实例化」的：调用方填好引脚与增益，两路 HX711（握力#2、体重#1）
 * 各持一个 hx711_t 即可，互不干扰。上层模块只管标定与语义，时序全在这里。
 *
 * 时序要点（据数据手册）：
 *   - DOUT 变低表示一次转换就绪；随后每个 SCK 上升沿移出一位（MSB 先出）。
 *   - 24 个脉冲读完数据后，再补 1~3 个脉冲选定「下次」转换的通道与增益。
 *   - SCK 高电平若持续 >60us 会触发掉电，故读时序用 __disable_irq 包裹，
 *     防止中断把某个 SCK 高相位拉长导致误掉电。
 */
#ifndef BSP_HX711_H
#define BSP_HX711_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

/** @brief 增益/通道：数值即读完 24bit 后要补足的 SCK 总脉冲数。 */
typedef enum {
    HX711_GAIN_A128 = 25,  /* 通道 A，增益 128（最灵敏，称重常用） */
    HX711_GAIN_B32  = 26,  /* 通道 B，增益 32 */
    HX711_GAIN_A64  = 27,  /* 通道 A，增益 64 */
} hx711_gain_t;

typedef struct {
    GPIO_TypeDef *sck_port;
    uint16_t      sck_pin;
    GPIO_TypeDef *dout_port;
    uint16_t      dout_pin;
    hx711_gain_t  gain;
    int32_t       offset;   /* 皮重（tare），净读数 = raw - offset */
} hx711_t;

/** @brief 配置 SCK 推挽输出、DOUT 上拉输入并上电。须在 delay_init 之后调用。 */
void hx711_init(hx711_t *h);

/** @brief DOUT 是否已就绪（低电平）。返回 1 就绪、0 未就绪。 */
int hx711_is_ready(const hx711_t *h);

/**
 * @brief 阻塞读一次原始值（24bit 有符号，已符号扩展）。
 * @param out        写入原始读数（未减皮重）
 * @param timeout_ms 等待就绪的最长毫秒数
 * @return 0 成功；-1 超时（DOUT 一直不就绪）
 */
int hx711_read_raw(hx711_t *h, int32_t *out, uint32_t timeout_ms);

/**
 * @brief 连读 times 次取平均，抑制随机噪声。
 * @return 0 成功；-1 任一次读超时
 */
int hx711_read_average(hx711_t *h, uint8_t times, int32_t *out, uint32_t timeout_ms);

/**
 * @brief 去皮：多次平均当前读数并存为 offset（调用时秤上应空载）。
 * @return 0 成功；-1 读超时
 */
int hx711_tare(hx711_t *h, uint8_t times, uint32_t timeout_ms);

/** @brief 掉电（SCK 拉高 >60us）。 */
void hx711_power_down(hx711_t *h);
/** @brief 上电（SCK 拉低）。 */
void hx711_power_up(hx711_t *h);

#endif /* BSP_HX711_H */
