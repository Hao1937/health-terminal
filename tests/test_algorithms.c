/**
 * @file    test_algorithms.c
 * @brief   BMI/体脂率、综合评分、平衡指数、心率血氧解算的基本正确性测试。
 * @owner   王宇浩（组长）
 */
#include "test_util.h"
#include "bodyfat_navy.h"
#include "health_score.h"
#include "balance.h"
#include "spo2_hr.h"
#include <string.h>
#include <math.h>

static void test_bmi(void)
{
    printf("test_bmi\n");
    int32_t bmi;
    /* 173.2cm, 65.3kg → BMI = 65.3/1.732^2 = 21.77 */
    CHECK_EQ(bmi_compute(1732, 65300, &bmi), HS_OK);
    CHECK_NEAR(bmi, 2177, 5);
    /* 非法身高被拒 */
    CHECK_EQ(bmi_compute(100, 65300, &bmi), HS_NOT_READY);
}

static void test_bodyfat(void)
{
    printf("test_bodyfat\n");
    int32_t bf;
    /* 男性 身高178cm 颈38cm 腰85cm → 海军公式约 18~20% */
    CHECK_EQ(bodyfat_navy(SEX_MALE, 1780, 380, 850, 0, &bf), HS_OK);
    CHECK(bf > 120 && bf < 260); /* 12.0%~26.0% 合理区间 */
    /* 腰<=颈 使对数域非法，被拒 */
    CHECK_EQ(bodyfat_navy(SEX_MALE, 1780, 400, 380, 0, &bf), HS_NOT_READY);
}

static void test_score(void)
{
    printf("test_score\n");
    measurement_record_t r;
    memset(&r, 0, sizeof(r));
    /* 全部落在理想区间 → 满分 100 */
    r.bmi_x100 = 2100; r.bodyfat_x10 = 150; r.heart_rate_bpm = 72;
    r.spo2_x10 = 985;  r.balance_x10 = 10;  r.reaction_ms = 220;
    int32_t s;
    CHECK_EQ(health_score_compute(&r, &s), HS_OK);
    CHECK_EQ(s, 100);

    /* 全部无效 → 无法评分 */
    r.bmi_x100 = HS_VALUE_INVALID; r.bodyfat_x10 = HS_VALUE_INVALID;
    r.heart_rate_bpm = HS_VALUE_INVALID; r.spo2_x10 = HS_VALUE_INVALID;
    r.balance_x10 = HS_VALUE_INVALID; r.reaction_ms = HS_VALUE_INVALID;
    CHECK_EQ(health_score_compute(&r, &s), HS_NOT_READY);

    /* 偏离理想应低于满分 */
    memset(&r, 0, sizeof(r));
    r.bmi_x100 = 3200; r.bodyfat_x10 = 350; r.heart_rate_bpm = 130;
    r.spo2_x10 = 900;  r.balance_x10 = 200; r.reaction_ms = 600;
    CHECK_EQ(health_score_compute(&r, &s), HS_OK);
    CHECK(s < 60);
}

static void test_balance(void)
{
    printf("test_balance\n");
    int32_t idx;
    /* 完全静止（角度恒定）→ 指数 0 */
    int32_t flat_p[8] = {500,500,500,500,500,500,500,500};
    int32_t flat_r[8] = {0,0,0,0,0,0,0,0};
    CHECK_EQ(balance_index(flat_p, flat_r, 8, &idx), HS_OK);
    CHECK_EQ(idx, 0);
    /* 大幅晃动 → 指数明显更大 */
    int32_t sway_p[8] = {-500,500,-500,500,-500,500,-500,500};
    CHECK_EQ(balance_index(sway_p, flat_r, 8, &idx), HS_OK);
    CHECK(idx > 40);
}

static void test_spo2_hr(void)
{
    printf("test_spo2_hr\n");
    /* 合成 100Hz、约 75bpm(=1.25Hz) 的正弦搏动，IR/RED 带直流 */
    enum { N = 300 };
    static int32_t ir[N], red[N];
    for (int i = 0; i < N; ++i) {
        double t = (double)i / 100.0;
        double beat = sin(2.0 * 3.14159265 * 1.25 * t);
        ir[i]  = (int32_t)(100000 + 800 * beat);
        red[i] = (int32_t)(90000  + 500 * beat);
    }
    spo2_hr_result_t res;
    hs_status_t st = spo2_hr_compute(ir, red, N, 100, &res);
    CHECK_EQ(st, HS_OK);
    CHECK(res.valid == 1);
    CHECK_NEAR(res.heart_rate_bpm, 75, 8); /* 峰检测允许 ±8bpm 误差 */
    CHECK(res.spo2_x10 >= 700 && res.spo2_x10 <= 1000);

    /* 样本不足直接 NOT_READY */
    CHECK_EQ(spo2_hr_compute(ir, red, 10, 100, &res), HS_NOT_READY);
}

int main(void)
{
    printf("== test_algorithms ==\n");
    test_bmi();
    test_bodyfat();
    test_score();
    test_balance();
    test_spo2_hr();
    return TEST_SUMMARY();
}
