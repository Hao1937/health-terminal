/**
 * @file    hx711_grip.c
 * @owner   王宇浩（组长）
 *
 * TODO(王宇浩（组长）)：同 HX711 时序，标定为 kg×10 写 primary，峰值保持取握力最大值
 */
#include "hx711_grip.h"

#if defined(MODULE_ENABLED_HX711_GRIP)

hs_status_t hx711_grip_init(void)
{
    /* TODO(王宇浩（组长）)：外设上电、寄存器配置、自检 */
    return HS_NOT_IMPLEMENTED;
}

hs_status_t hx711_grip_measure(hs_sample_t *out)
{
    /* TODO(王宇浩（组长）)：采集一次并写 out->primary（双输出再写 out->secondary） */
    (void)out;
    return HS_NOT_IMPLEMENTED;
}

#else  /* 本板 MODULE_SET 未包含该模块：桩替代，仅供 app 注册表链接 */

hs_status_t hx711_grip_init(void) { return HS_NOT_IMPLEMENTED; }
hs_status_t hx711_grip_measure(hs_sample_t *out) { (void)out; return HS_NOT_IMPLEMENTED; }

#endif
