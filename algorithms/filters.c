/**
 * @file    filters.c
 * @owner   王宇浩（组长）
 */
#include "filters.h"

/* ============================ 中值滤波 ============================ */

void median_init(median_filter_t *f, uint8_t window)
{
    if (window < 1) window = 1;
    if (window > MEDIAN_MAX_WINDOW) window = MEDIAN_MAX_WINDOW;
    f->window = window;
    f->count = 0;
    f->head = 0;
    for (uint8_t i = 0; i < MEDIAN_MAX_WINDOW; ++i) f->buf[i] = 0;
}

int32_t median_push(median_filter_t *f, int32_t sample)
{
    /* 写入环形缓冲 */
    f->buf[f->head] = sample;
    f->head = (uint8_t)((f->head + 1) % f->window);
    if (f->count < f->window) f->count++;

    /* 拷贝有效样本并插入排序（窗口小，O(n^2) 足够） */
    int32_t tmp[MEDIAN_MAX_WINDOW];
    uint8_t n = f->count;
    for (uint8_t i = 0; i < n; ++i) tmp[i] = f->buf[i];
    for (uint8_t i = 1; i < n; ++i) {
        int32_t key = tmp[i];
        int j = (int)i - 1;
        while (j >= 0 && tmp[j] > key) { tmp[j + 1] = tmp[j]; --j; }
        tmp[j + 1] = key;
    }
    return tmp[n / 2];
}

/* ============================ 滑动平均 ============================ */

void movavg_init(movavg_filter_t *f, uint8_t window)
{
    if (window < 1) window = 1;
    if (window > MOVAVG_MAX_WINDOW) window = MOVAVG_MAX_WINDOW;
    f->window = window;
    f->count = 0;
    f->head = 0;
    f->sum = 0;
    for (uint8_t i = 0; i < MOVAVG_MAX_WINDOW; ++i) f->buf[i] = 0;
}

int32_t movavg_push(movavg_filter_t *f, int32_t sample)
{
    if (f->count == f->window) {
        /* 窗口已满：减去被覆盖的旧样本 */
        f->sum -= f->buf[f->head];
    } else {
        f->count++;
    }
    f->buf[f->head] = sample;
    f->sum += sample;
    f->head = (uint8_t)((f->head + 1) % f->window);

    /* 四舍五入到最近整数 */
    int64_t n = f->count;
    int64_t s = f->sum;
    if (s >= 0) return (int32_t)((s + n / 2) / n);
    return (int32_t)((s - n / 2) / n);
}

/* ============================ 互补滤波 ============================ */

void comp_init(comp_filter_t *f, float alpha)
{
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;
    f->alpha = alpha;
    f->angle = 0.0f;
    f->inited = 0;
}

float comp_update(comp_filter_t *f, float acc_angle, float gyro_rate, float dt)
{
    if (!f->inited) {
        /* 首帧用加速度计角作为初值，避免从 0 缓慢收敛 */
        f->angle = acc_angle;
        f->inited = 1;
        return f->angle;
    }
    float predicted = f->angle + gyro_rate * dt;
    f->angle = f->alpha * predicted + (1.0f - f->alpha) * acc_angle;
    return f->angle;
}
