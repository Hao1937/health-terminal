/**
 * @file    hx711_weight.c
 * @owner   查樊听
 *
 * TODO(查樊听)：SCK/DOUT 软件时序读 24bit，减皮重×标定系数得克数写 primary，稳定判据不满足返回 HS_UNSTABLE
 */
#include "hx711_weight.h"

#if defined(MODULE_ENABLED_HX711_WEIGHT)

hs_status_t hx711_weight_init(void)
{
    /* TODO(查樊听)：外设上电、寄存器配置、自检 */
    return HS_NOT_IMPLEMENTED;
}

hs_status_t hx711_weight_measure(hs_sample_t *out)
{
    /* TODO(查樊听)：采集一次并写 out->primary（双输出再写 out->secondary） */
    (void)out;
    return HS_NOT_IMPLEMENTED;
}

#else  /* 本板 MODULE_SET 未包含该模块：桩替代，仅供 app 注册表链接 */

hs_status_t hx711_weight_init(void) { return HS_NOT_IMPLEMENTED; }
hs_status_t hx711_weight_measure(hs_sample_t *out) { (void)out; return HS_NOT_IMPLEMENTED; }

#endif
