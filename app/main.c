/**
 * @file    main.c
 * @brief   固件入口：时钟初始化 + LED 心跳 + 调试串口打印版本号 + 主状态机。
 * @owner   王宇浩（组长）
 *
 * 首版固件（全模块桩）克隆当天即可烧录：应看到 PC13 LED 闪烁，且 USART1
 * (115200 8N1) 打印版本号与自检信息，验证工具链与板子通路正常。
 */
#include "stm32f1xx_hal.h"
#include "app.h"

/* 在 core/src/system_clock.c 中定义 */
void SystemClock_Config(void);

int main(void)
{
    HAL_Init();               /* 复位外设、初始化 SysTick 时基 */
    SystemClock_Config();     /* 8MHz HSE → 72MHz */

    app_init();               /* 外设/模块初始化 + 打印版本号 */

    while (1) {
        app_tick();           /* 裸机主循环推进状态机 */
    }
}
