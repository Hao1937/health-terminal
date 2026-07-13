/**
 * @file    reaction.c
 * @owner   刘晏铭
 *
 * TODO(刘晏铭)：亮 LED 记 t0，EXTI 按键中断记 t1，反应时间=t1-t0 写 primary(ms)，抢跑返回 HS_UNSTABLE
 */
#include "reaction.h"

#if defined(MODULE_ENABLED_REACTION)

hs_status_t reaction_init(void)
{
    /* TODO(刘晏铭)：外设上电、寄存器配置、自检 */
    return HS_NOT_IMPLEMENTED;
}

hs_status_t reaction_measure(hs_sample_t *out)
{
    /* TODO(刘晏铭)：采集一次并写 out->primary（双输出再写 out->secondary） */
    (void)out;
    return HS_NOT_IMPLEMENTED;
}

#else  /* 本板 MODULE_SET 未包含该模块：桩替代，仅供 app 注册表链接 */

hs_status_t reaction_init(void) { return HS_NOT_IMPLEMENTED; }
hs_status_t reaction_measure(hs_sample_t *out) { (void)out; return HS_NOT_IMPLEMENTED; }

#endif
