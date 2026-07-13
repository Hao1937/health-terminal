/**
 * @file    filters.h
 * @brief   公共滤波库：中值滤波、滑动平均、MPU 姿态互补滤波。
 * @owner   王宇浩（组长）
 *
 * 纯 C、定长静态缓冲、无动态内存、无硬件依赖，可在 PC 编译与单元测试，
 * 也可在 STM32 上直接使用。整型/定点为主，互补滤波用 float（仅姿态解算，
 * F103 无 FPU 但调用频率低，-Os 下可接受）。
 */
#ifndef FILTERS_H
#define FILTERS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/*  中值滤波：对最近 N 个样本取中值，去除脉冲毛刺。                     */
/* ------------------------------------------------------------------ */
#define MEDIAN_MAX_WINDOW 15

typedef struct {
    int32_t buf[MEDIAN_MAX_WINDOW]; /* 环形缓冲，存最近 window 个样本 */
    uint8_t window;                 /* 实际窗口大小，奇数为宜 */
    uint8_t count;                  /* 已填入样本数（预热用） */
    uint8_t head;                   /* 下一个写入位置 */
} median_filter_t;

/** @brief 初始化中值滤波器，window 会被夹到 [1, MEDIAN_MAX_WINDOW]。 */
void median_init(median_filter_t *f, uint8_t window);
/** @brief 送入一个样本，返回当前窗口内样本的中值。 */
int32_t median_push(median_filter_t *f, int32_t sample);

/* ------------------------------------------------------------------ */
/*  滑动平均：最近 N 个样本的算术平均，平滑噪声。                       */
/* ------------------------------------------------------------------ */
#define MOVAVG_MAX_WINDOW 32

typedef struct {
    int32_t buf[MOVAVG_MAX_WINDOW];
    int64_t sum;      /* 窗口内样本和，避免每次重算 */
    uint8_t window;
    uint8_t count;
    uint8_t head;
} movavg_filter_t;

/** @brief 初始化滑动平均，window 会被夹到 [1, MOVAVG_MAX_WINDOW]。 */
void movavg_init(movavg_filter_t *f, uint8_t window);
/** @brief 送入样本，返回当前平均值（四舍五入到整数）。 */
int32_t movavg_push(movavg_filter_t *f, int32_t sample);

/* ------------------------------------------------------------------ */
/*  互补滤波：融合加速度计静态角与陀螺积分角，估计俯仰/横滚角。         */
/*  angle = alpha*(angle + gyro*dt) + (1-alpha)*acc_angle              */
/* ------------------------------------------------------------------ */
typedef struct {
    float angle;   /* 当前融合角，单位度 */
    float alpha;   /* 陀螺权重，典型 0.95~0.98 */
    uint8_t inited;
} comp_filter_t;

/** @brief 初始化互补滤波器，alpha 会被夹到 [0,1]。 */
void comp_init(comp_filter_t *f, float alpha);
/**
 * @brief 融合一步。
 * @param acc_angle 加速度计解算的静态角（度）
 * @param gyro_rate 陀螺角速度（度/秒）
 * @param dt        采样间隔（秒）
 * @return 融合后角度（度）。首帧直接采用 acc_angle 作为初值。
 */
float comp_update(comp_filter_t *f, float acc_angle, float gyro_rate, float dt);

#ifdef __cplusplus
}
#endif

#endif /* FILTERS_H */
