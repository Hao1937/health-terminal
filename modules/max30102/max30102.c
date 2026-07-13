/**
 * @file    max30102.c
 * @owner   王宇浩（组长）
 *
 * MAX30102 心率血氧（I2C 0x57）。驱动只负责「配置 + 从 FIFO 取一段 RED/IR 原始样本」，
 * 心率/血氧解算交给 algorithms/spo2_hr（与硬件解耦、可在 PC 回放测试）。
 *
 * 配置：SpO2 模式（RED+IR 双通道），采样率 400Hz + 4 点平均 = 有效 100Hz，18bit 分辨率。
 * measure() 清空 FIFO 后连续取样约 2s，再交 spo2_hr_compute 得心率与 SpO2。
 */
#include "max30102.h"

#if defined(MODULE_ENABLED_MAX30102)

#include "i2c_bus.h"
#include "board.h"
#include "spo2_hr.h"
#include "stm32f1xx_hal.h"

/* ---- 寄存器地址（数据手册 Table） ---- */
#define REG_INTR_STATUS_1  0x00
#define REG_FIFO_WR_PTR    0x04
#define REG_OVF_COUNTER    0x05
#define REG_FIFO_RD_PTR    0x06
#define REG_FIFO_DATA      0x07
#define REG_FIFO_CONFIG    0x08
#define REG_MODE_CONFIG    0x09
#define REG_SPO2_CONFIG    0x0A
#define REG_LED1_PA        0x0C   /* RED */
#define REG_LED2_PA        0x0D   /* IR  */
#define REG_PART_ID        0xFF

#define PART_ID_MAX30102   0x15
#define MODE_RESET         0x40
#define MODE_SPO2          0x03

#define MAX30102_ADDR      I2C_ADDR_MAX30102   /* board.h 中已左移 */
#define FIFO_DEPTH         32u                 /* 32 样本深 FIFO */
#define BYTES_PER_SAMPLE   6u                  /* SpO2 模式：RED(3)+IR(3) */

#define SAMPLE_RATE_HZ     100u                /* 有效 FIFO 速率（400Hz/4） */
#define TARGET_SAMPLES     200u                /* 约 2s，覆盖数个心动周期 */
#define MIN_SAMPLES        64u                 /* 少于此认为没放手指/信号太短 */
#define MEASURE_TIMEOUT_MS 4000u

static int wr_reg(uint8_t reg, uint8_t val)
{
    return i2c_mem_write(MAX30102_ADDR, reg, &val, 1);
}
static int rd_reg(uint8_t reg, uint8_t *val)
{
    return i2c_mem_read(MAX30102_ADDR, reg, val, 1);
}

/* 清零 FIFO 三个指针，使下一次读从新鲜数据开始 */
static int fifo_reset(void)
{
    int rc = 0;
    rc |= wr_reg(REG_FIFO_WR_PTR, 0x00);
    rc |= wr_reg(REG_OVF_COUNTER, 0x00);
    rc |= wr_reg(REG_FIFO_RD_PTR, 0x00);
    return rc;
}

hs_status_t max30102_init(void)
{
    uint8_t id = 0;
    if (rd_reg(REG_PART_ID, &id) != 0) return HS_TIMEOUT;   /* I2C 无应答 */
    if (id != PART_ID_MAX30102)        return HS_TIMEOUT;   /* 器件不对 */

    /* 软复位并等待复位位自清 */
    if (wr_reg(REG_MODE_CONFIG, MODE_RESET) != 0) return HS_TIMEOUT;
    uint32_t t0 = HAL_GetTick();
    do {
        if (rd_reg(REG_MODE_CONFIG, &id) != 0) return HS_TIMEOUT;
        if (HAL_GetTick() - t0 > 100u) return HS_TIMEOUT;
    } while (id & MODE_RESET);

    int rc = 0;
    rc |= fifo_reset();
    /* FIFO_CONFIG：采样平均=4(0b010<<5)、rollover 使能(1<<4)、almost-full=17(0x0F) */
    rc |= wr_reg(REG_FIFO_CONFIG, (0x02u << 5) | (1u << 4) | 0x0Fu);
    /* MODE_CONFIG：SpO2 模式（RED+IR） */
    rc |= wr_reg(REG_MODE_CONFIG, MODE_SPO2);
    /* SPO2_CONFIG：ADC 量程 4096nA(0b01<<5)、采样率 400Hz(0b011<<2)、脉宽 411us/18bit(0b11) */
    rc |= wr_reg(REG_SPO2_CONFIG, (0x01u << 5) | (0x03u << 2) | 0x03u);
    /* LED 电流：RED/IR 各约 7mA（0x24 步进 0.2mA），指尖测量足够 */
    rc |= wr_reg(REG_LED1_PA, 0x24);
    rc |= wr_reg(REG_LED2_PA, 0x24);
    rc |= fifo_reset();
    if (rc != 0) return HS_TIMEOUT;

    return HS_OK;
}

hs_status_t max30102_measure(hs_sample_t *out)
{
    /* 样本缓冲放静态区，避免占用主循环栈（200*4*2=1600B） */
    static int32_t ir_buf[TARGET_SAMPLES];
    static int32_t red_buf[TARGET_SAMPLES];

    out->primary   = HS_VALUE_INVALID;
    out->secondary = HS_VALUE_INVALID;

    if (fifo_reset() != 0) return HS_TIMEOUT;

    size_t count = 0;
    uint32_t t0 = HAL_GetTick();
    while (count < TARGET_SAMPLES) {
        if (HAL_GetTick() - t0 > MEASURE_TIMEOUT_MS) break;

        uint8_t wr = 0, rd = 0;
        if (rd_reg(REG_FIFO_WR_PTR, &wr) != 0) return HS_TIMEOUT;
        if (rd_reg(REG_FIFO_RD_PTR, &rd) != 0) return HS_TIMEOUT;
        int avail = ((int)wr - (int)rd) & (int)(FIFO_DEPTH - 1);   /* 环形，取模 32 */
        if (avail == 0) { HAL_Delay(5); continue; }

        for (int k = 0; k < avail && count < TARGET_SAMPLES; ++k) {
            uint8_t d[BYTES_PER_SAMPLE];
            if (i2c_mem_read(MAX30102_ADDR, REG_FIFO_DATA, d, BYTES_PER_SAMPLE) != 0) {
                return HS_TIMEOUT;
            }
            /* 每通道 18bit，高位在前；SpO2 模式 FIFO 顺序为 RED 后 IR */
            uint32_t red = (((uint32_t)d[0] << 16) | ((uint32_t)d[1] << 8) | d[2]) & 0x03FFFFu;
            uint32_t ir  = (((uint32_t)d[3] << 16) | ((uint32_t)d[4] << 8) | d[5]) & 0x03FFFFu;
            red_buf[count] = (int32_t)red;
            ir_buf[count]  = (int32_t)ir;
            ++count;
        }
    }

    if (count < MIN_SAMPLES) return HS_NOT_READY;   /* 采样太少：多半没放手指 */

    spo2_hr_result_t res;
    hs_status_t st = spo2_hr_compute(ir_buf, red_buf, count, SAMPLE_RATE_HZ, &res);
    if (st != HS_OK) return st;                      /* 传播 UNSTABLE/NOT_READY */

    out->primary   = res.heart_rate_bpm;
    out->secondary = res.spo2_x10;
    return HS_OK;
}

#else  /* 本板 MODULE_SET 未包含该模块：桩替代，仅供 app 注册表链接 */

hs_status_t max30102_init(void) { return HS_NOT_IMPLEMENTED; }
hs_status_t max30102_measure(hs_sample_t *out) { (void)out; return HS_NOT_IMPLEMENTED; }

#endif
