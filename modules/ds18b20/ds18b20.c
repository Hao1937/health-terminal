/**
 * @file    ds18b20.c
 * @owner   查樊听
 *
 * TODO(查樊听)：单总线复位/ROM 跳过/启动转换/读 2 字节，温度×?写 primary(0.01℃)，供 hcsr04 声速补偿
 */
#include "ds18b20.h"

#if defined(MODULE_ENABLED_DS18B20)

hs_status_t ds18b20_init(void)
{
    /* TODO(查樊听)：外设上电、寄存器配置、自检 */
    return HS_NOT_IMPLEMENTED;
}

hs_status_t ds18b20_measure(hs_sample_t *out)
{
    /* TODO(查樊听)：采集一次并写 out->primary（双输出再写 out->secondary） */
    (void)out;
    return HS_NOT_IMPLEMENTED;
}

#else  /* 本板 MODULE_SET 未包含该模块：桩替代，仅供 app 注册表链接 */

hs_status_t ds18b20_init(void) { return HS_NOT_IMPLEMENTED; }
hs_status_t ds18b20_measure(hs_sample_t *out) { (void)out; return HS_NOT_IMPLEMENTED; }

#endif
