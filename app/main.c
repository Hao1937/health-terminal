/**
 * @file    main.c
 * @brief   固件入口：时钟初始化 + LED 心跳 + 调试串口打印版本号 + 主状态机。
 * @owner   王宇浩（组长）
 *
 * 首版固件（全模块桩）克隆当天即可烧录：应看到 PC13 LED 闪烁，且 USART1
 * (115200 8N1) 打印版本号与自检信息，验证工具链与板子通路正常。
 */
#include "app.h"
#include "stm32f1xx_hal.h"

#if defined(YUHAO_BRINGUP)
#include <stdio.h>

#include "board.h"
#include "health_if.h"
#include "hx711_grip.h"
#include "i2c_bus.h"
#include "max30102.h"
#endif

/* 在 core/src/system_clock.c 中定义 */
void SystemClock_Config(void);

#if defined(YUHAO_BRINGUP)
static void yuhao_bringup_once(void) {
  static uint32_t pass;
  hs_sample_t sample = {HS_VALUE_INVALID, HS_VALUE_INVALID};
  hs_status_t st;

  printf("\r\n[yuhao] ===== bring-up pass %lu =====\r\n",
         (unsigned long)++pass);

  /* app_init() 会探测整机的 OLED/MPU6050/MAX30102；个人子集没有前两者时，
   * 某些 F1 HAL 版本可能把 I2C 外设留在 BUSY 状态。每轮测试前复位外设，
   * 并记录总线空闲电平与器件 ACK，避免把软件状态误判成传感器故障。 */
  i2c_bus_recover();
  i2c_bus_init();
  printf("[yuhao] I2C lines SCL=%d SDA=%d MAX30102_ACK=%d\r\n",
         HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6),
         HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7),
         i2c_probe(I2C_ADDR_MAX30102));
  i2c_bus_init();

  printf("\r\n[yuhao] MAX30102 init...\r\n");
  st = max30102_init();
  printf("[yuhao] max30102_init -> %s\r\n", hs_status_str(st));
  if (st == HS_OK) {
    st = max30102_measure(&sample);
    printf("[yuhao] max30102_measure -> %s primary=%ld secondary=%ld\r\n",
           hs_status_str(st), (long)sample.primary, (long)sample.secondary);
  }

  sample.primary = HS_VALUE_INVALID;
  sample.secondary = HS_VALUE_INVALID;
  printf("[yuhao] HX711 grip init...\r\n");
  st = hx711_grip_init();
  printf("[yuhao] hx711_grip_init -> %s\r\n", hs_status_str(st));
  if (st == HS_OK) {
    st = hx711_grip_measure(&sample);
    printf("[yuhao] hx711_grip_measure -> %s primary=%ld secondary=%ld\r\n",
           hs_status_str(st), (long)sample.primary, (long)sample.secondary);
  }
  printf("[yuhao] bring-up pass complete\r\n");
}
#endif

int main(void) {
  HAL_Init();           /* 复位外设、初始化 SysTick 时基 */
  SystemClock_Config(); /* 8MHz HSE → 72MHz */

  app_init(); /* 外设/模块初始化 + 打印版本号 */

#if defined(YUHAO_BRINGUP)
  printf("[yuhao] bring-up mode enabled\r\n");
  uint32_t next_bringup_ms = HAL_GetTick();
  while (1) {
    app_tick(); /* 保留 LED 心跳，证明主循环仍在运行 */
    uint32_t now = HAL_GetTick();
    if ((int32_t)(now - next_bringup_ms) >= 0) {
      yuhao_bringup_once();
      /* 一轮测量结束后留出间隔，避免传感器被无间隔重复初始化。 */
      next_bringup_ms = HAL_GetTick() + 2000U;
    }
  }
#else
  while (1) {
    app_tick(); /* 裸机主循环推进状态机 */
  }
#endif
}
