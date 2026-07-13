/**
 * @file    max30102.c
 * @owner   王宇浩（组长）
 *
 * TODO(王宇浩（组长）)：读 FIFO 得 RED/IR 缓冲，调 spo2_hr_compute 得心率血氧，写 primary=心率 secondary=SpO2x10
 */
#include "max30102.h"

#if defined(MODULE_ENABLED_MAX30102)

hs_status_t max30102_init(void)
{
    /* TODO(王宇浩（组长）)：外设上电、寄存器配置、自检 */
    return HS_NOT_IMPLEMENTED;
}

hs_status_t max30102_measure(hs_sample_t *out)
{
    /* TODO(王宇浩（组长）)：采集一次并写 out->primary（双输出再写 out->secondary） */
    (void)out;
    return HS_NOT_IMPLEMENTED;
}

#else  /* 本板 MODULE_SET 未包含该模块：桩替代，仅供 app 注册表链接 */

hs_status_t max30102_init(void) { return HS_NOT_IMPLEMENTED; }
hs_status_t max30102_measure(hs_sample_t *out) { (void)out; return HS_NOT_IMPLEMENTED; }

#endif
