/**
 * @file    hx711.c
 * @owner   王宇浩（组长）
 */
#include "hx711.h"

#include "delay.h"

/* 使能某 GPIO 端口时钟（board_gpio_init 已开
 * A/B/C，这里再做一次保证自洽、幂等） */
static void gpio_clk_enable(GPIO_TypeDef *port) {
  if (port == GPIOA)
    __HAL_RCC_GPIOA_CLK_ENABLE();
  else if (port == GPIOB)
    __HAL_RCC_GPIOB_CLK_ENABLE();
  else if (port == GPIOC)
    __HAL_RCC_GPIOC_CLK_ENABLE();
}

static inline void sck_high(const hx711_t *h) {
  HAL_GPIO_WritePin(h->sck_port, h->sck_pin, GPIO_PIN_SET);
}
static inline void sck_low(const hx711_t *h) {
  HAL_GPIO_WritePin(h->sck_port, h->sck_pin, GPIO_PIN_RESET);
}

void hx711_init(hx711_t *h) {
  gpio_clk_enable(h->sck_port);
  gpio_clk_enable(h->dout_port);

  GPIO_InitTypeDef g = {0};
  /* SCK：推挽输出 */
  g.Pin = h->sck_pin;
  g.Mode = GPIO_MODE_OUTPUT_PP;
  g.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(h->sck_port, &g);

  /* DOUT：上拉输入（空闲/掉线时读到高 = 未就绪，安全） */
  g.Pin = h->dout_pin;
  g.Mode = GPIO_MODE_INPUT;
  g.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(h->dout_port, &g);

  h->offset = 0;
  hx711_power_up(h);
}

int hx711_is_ready(const hx711_t *h) {
  return (HAL_GPIO_ReadPin(h->dout_port, h->dout_pin) == GPIO_PIN_RESET) ? 1
                                                                         : 0;
}

void hx711_power_up(hx711_t *h) {
  sck_low(h);
  delay_us(60); /* 掉电恢复后需一段稳定时间 */
}

void hx711_power_down(hx711_t *h) {
  sck_low(h);
  delay_us(1);
  sck_high(h);
  delay_us(80); /* SCK 保持高 >60us 进入掉电 */
}

int hx711_read_raw(hx711_t *h, int32_t *out, uint32_t timeout_ms) {
  /* 等待 DOUT 变低（就绪） */
  uint32_t t0 = HAL_GetTick();
  while (!hx711_is_ready(h)) {
    if (HAL_GetTick() - t0 >= timeout_ms) return -1;
  }

  uint32_t value = 0;

  /* 关中断，避免 SCK 高相位被拉长(>60us)误触发掉电；用 PRIMASK
   * 保存恢复，可嵌套安全 */
  uint32_t primask = __get_PRIMASK();
  __disable_irq();

  /* 24 个脉冲：每个上升沿后读一位，MSB 先出 */
  for (int i = 0; i < 24; ++i) {
    sck_high(h);
    delay_us(1);
    uint32_t bit =
        (HAL_GPIO_ReadPin(h->dout_port, h->dout_pin) == GPIO_PIN_SET) ? 1u : 0u;
    value = (value << 1) | bit;
    sck_low(h);
    delay_us(1);
  }
  /* 补足增益/通道脉冲：gain(25/26/27) - 已发的 24 */
  for (int i = 24; i < (int)h->gain; ++i) {
    sck_high(h);
    delay_us(1);
    sck_low(h);
    delay_us(1);
  }

  __set_PRIMASK(primask);

  /* 24bit 补码符号扩展到 32bit */
  if (value & 0x00800000u) value |= 0xFF000000u;
  *out = (int32_t)value;
  return 0;
}

int hx711_read_average(hx711_t *h, uint8_t times, int32_t *out,
                       uint32_t timeout_ms) {
  if (times == 0) times = 1;
  int64_t sum = 0;
  for (uint8_t i = 0; i < times; ++i) {
    int32_t v;
    if (hx711_read_raw(h, &v, timeout_ms) != 0) return -1;
    sum += v;
  }
  *out = (int32_t)(sum / times);
  return 0;
}

int hx711_tare(hx711_t *h, uint8_t times, uint32_t timeout_ms) {
  int32_t avg;
  if (hx711_read_average(h, times, &avg, timeout_ms) != 0) return -1;
  h->offset = avg;
  return 0;
}
