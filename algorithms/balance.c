/**
 * @file    balance.c
 * @owner   刘晏铭
 */
#include "balance.h"

/* 计算定点序列的标准差（返回同量纲 ×100 的整数近似） */
static int32_t stddev_x100(const int32_t *b, size_t n)
{
    int64_t sum = 0;
    for (size_t i = 0; i < n; ++i) sum += b[i];
    int64_t mean = sum / (int64_t)n;

    int64_t var = 0;
    for (size_t i = 0; i < n; ++i) {
        int64_t d = (int64_t)b[i] - mean;
        var += d * d;
    }
    var /= (int64_t)n;

    /* 整数平方根（牛顿法） */
    int64_t x = var, y = (x + 1) / 2;
    if (x == 0) return 0;
    while (y < x) { x = y; y = (x + var / x) / 2; }
    return (int32_t)x; /* 与输入同为 ×100 量纲 */
}

hs_status_t balance_index(const int32_t *pitch_x100, const int32_t *roll_x100,
                          size_t n, int32_t *out_x10)
{
    if (!pitch_x100 || !roll_x100 || !out_x10) return HS_NOT_READY;
    if (n < 4) return HS_NOT_READY;

    int32_t sp = stddev_x100(pitch_x100, n); /* 度 ×100 */
    int32_t sr = stddev_x100(roll_x100, n);

    /* 指数 = (σpitch + σroll)，从 ×100 折算到 ×10 输出 */
    int32_t idx_x100 = sp + sr;
    *out_x10 = idx_x100 / 10;
    return HS_OK;
}
