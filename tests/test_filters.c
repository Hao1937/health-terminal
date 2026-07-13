/**
 * @file    test_filters.c
 * @brief   滤波公共库单元测试：中值去毛刺、滑动平均收敛、互补滤波静态角。
 * @owner   王宇浩（组长）
 */
#include "test_util.h"
#include "filters.h"
#include <math.h>

/* 用例 1：中值滤波去除单点脉冲毛刺 */
static void test_median_despike(void)
{
    printf("test_median_despike\n");
    median_filter_t f;
    median_init(&f, 5);
    /* 平稳基线 100，中间混入一个 9999 的毛刺 */
    int32_t in[]  = {100, 100, 100, 9999, 100, 100, 100};
    int32_t last = 0;
    for (int i = 0; i < 7; ++i) last = median_push(&f, in[i]);
    /* 窗口填满后，中值应把毛刺完全抑制，稳定在 100 */
    CHECK_EQ(last, 100);

    /* 单独验证毛刺进入瞬间也不会让输出跳到 9999 */
    median_init(&f, 5);
    median_push(&f, 100); median_push(&f, 100);
    int32_t y = median_push(&f, 9999); /* 3 个样本 {100,100,9999} 的中值=100 */
    CHECK_EQ(y, 100);
}

/* 用例 2：滑动平均对阶跃输入单调收敛到新稳态 */
static void test_movavg_converge(void)
{
    printf("test_movavg_converge\n");
    movavg_filter_t f;
    movavg_init(&f, 8);
    /* 先喂 0 预热，再阶跃到 1000，输出应从 0 单调升到 1000 且不过冲 */
    for (int i = 0; i < 8; ++i) movavg_push(&f, 0);
    int32_t prev = 0, y = 0;
    for (int i = 0; i < 8; ++i) {
        y = movavg_push(&f, 1000);
        CHECK(y >= prev);      /* 单调不降 */
        CHECK(y <= 1000);      /* 不过冲 */
        prev = y;
    }
    CHECK_EQ(y, 1000);         /* 窗口被 1000 填满后完全收敛 */

    /* 平均值正确性：窗口 4，输入 10,20,30,40 → 平均 25 */
    movavg_init(&f, 4);
    movavg_push(&f, 10); movavg_push(&f, 20); movavg_push(&f, 30);
    CHECK_EQ(movavg_push(&f, 40), 25);
}

/* 用例 3：互补滤波在静止（陀螺=0）时收敛到加速度计静态角 */
static void test_comp_static(void)
{
    printf("test_comp_static\n");
    comp_filter_t f;
    comp_init(&f, 0.98f);
    /* 加速度计恒定报 30 度，陀螺无旋转。首帧取初值，之后保持 30 */
    float a = comp_update(&f, 30.0f, 0.0f, 0.01f);
    CHECK(fabsf(a - 30.0f) < 0.001f); /* 首帧直接采用 acc 角 */
    for (int i = 0; i < 200; ++i) a = comp_update(&f, 30.0f, 0.0f, 0.01f);
    CHECK(fabsf(a - 30.0f) < 0.01f);  /* 长期稳定在 30 度 */
}

/* 用例 4：互补滤波在纯陀螺积分下跟随角速度（acc 权重给 0） */
static void test_comp_gyro_integrate(void)
{
    printf("test_comp_gyro_integrate\n");
    comp_filter_t f;
    comp_init(&f, 1.0f);              /* alpha=1：完全信任陀螺积分 */
    comp_update(&f, 0.0f, 0.0f, 0.0f);/* 初值 0 */
    /* 以 10 度/秒转 1 秒（100 步 ×0.01s）→ 约 10 度 */
    float a = 0.0f;
    for (int i = 0; i < 100; ++i) a = comp_update(&f, 0.0f, 10.0f, 0.01f);
    CHECK(fabsf(a - 10.0f) < 0.05f);
}

int main(void)
{
    printf("== test_filters ==\n");
    test_median_despike();
    test_movavg_converge();
    test_comp_static();
    test_comp_gyro_integrate();
    return TEST_SUMMARY();
}
