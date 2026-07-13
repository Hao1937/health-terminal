/**
 * @file    hcsr04.c
 * @owner   查樊听
 *
 * TODO(查樊听)：Trig 10us 触发，TIM4 捕获回波脉宽换算距离，用 ds18b20 温度修正声速，站高-测距=身高写 primary(mm)
 */
#include "hcsr04.h"

#ifdef USE_HAL_DRIVER
#include "stm32f1xx_hal.h"
/* 供 core/src/stm32f1xx_it.c 的 TIM4_IRQHandler 转发回波捕获中断。
 * 恒定义（默认 NULL）：即使本模块为桩、TIM4 未启用，也保证固件链接通过。
 * 真实现时在 hcsr04_init 里指向本模块的 TIM4 句柄。 */
TIM_HandleTypeDef *g_hcsr04_htim = 0;
#endif

#if defined(MODULE_ENABLED_HCSR04)

hs_status_t hcsr04_init(void)
{
    /* TODO(查樊听)：外设上电、寄存器配置、自检 */
    return HS_NOT_IMPLEMENTED;
}

hs_status_t hcsr04_measure(hs_sample_t *out)
{
    /* TODO(查樊听)：采集一次并写 out->primary（双输出再写 out->secondary） */
    (void)out;
    return HS_NOT_IMPLEMENTED;
}

#else  /* 本板 MODULE_SET 未包含该模块：桩替代，仅供 app 注册表链接 */

hs_status_t hcsr04_init(void) { return HS_NOT_IMPLEMENTED; }
hs_status_t hcsr04_measure(hs_sample_t *out) { (void)out; return HS_NOT_IMPLEMENTED; }

#endif
