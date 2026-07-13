/**
 * @file    stm32f1xx_it.c
 * @brief   Cortex-M3 与外设中断服务程序。
 * @owner   王宇浩（组长）
 *
 * 反应时间按键(PB0/EXTI0)与 HC-SR04 回波(TIM4)在此转发到对应模块回调。
 */
#include "stm32f1xx_hal.h"
#include "board.h"

/* HC-SR04 使用的定时器句柄（bsp/模块中定义），用于回波捕获 */
extern TIM_HandleTypeDef *g_hcsr04_htim;

/* ---- Cortex-M3 内核异常 ---- */
void NMI_Handler(void)        { while (1) { } }
void HardFault_Handler(void)  { while (1) { } }
void MemManage_Handler(void)  { while (1) { } }
void BusFault_Handler(void)   { while (1) { } }
void UsageFault_Handler(void) { while (1) { } }
void SVC_Handler(void)        { }
void DebugMon_Handler(void)   { }
void PendSV_Handler(void)     { }

void SysTick_Handler(void)
{
    HAL_IncTick();   /* HAL 时基，HAL_Delay/超时依赖它 */
}

/* ---- 反应时间按键：PB0 → EXTI line0 ---- */
void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(REACTION_BTN_PIN);
}

/* ---- HC-SR04 回波捕获定时器（TIM4）---- */
void TIM4_IRQHandler(void)
{
    if (g_hcsr04_htim) {
        HAL_TIM_IRQHandler(g_hcsr04_htim);
    }
}
