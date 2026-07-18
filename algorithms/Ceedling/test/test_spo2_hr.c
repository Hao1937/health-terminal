#include <float.h>
#include <stdint.h>

#include "health_if.h"
#include "spo2_hr.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

void test_returns_not_ready_when_samples_too_few(void) {
  int32_t ir[10] = {100, 200, 300, 400, 500, 600, 700, 800, 900, 1000};
  int32_t red[10] = {100, 200, 300, 400, 500, 600, 700, 800, 900, 1000};
  spo2_hr_result_t res;
  hs_status_t st = spo2_hr_compute(ir, red, 10, 100, &res);
  TEST_ASSERT_EQUAL(HS_NOT_READY, st);
}

void test_returns_not_ready_when_sample_rate_is_zero(void) {
  int32_t ir[40] = {0};
  int32_t red[40] = {0};
  spo2_hr_result_t res;
  hs_status_t st = spo2_hr_compute(ir, red, 40, 0, &res);
  TEST_ASSERT_EQUAL(HS_NOT_READY, st);
}

void test_returns_unstable_when_ir_dc_is_zero(void) {
  int32_t ir[32] = {0};
  int32_t red[32];
  for (int i = 0; i < 32; i++) red[i] = 500;
  spo2_hr_result_t res;
  hs_status_t st = spo2_hr_compute(ir, red, 32, 100, &res);
  TEST_ASSERT_EQUAL(HS_UNSTABLE, st);
}

void test_returns_unstable_when_red_dc_is_zero(void) {
  int32_t ir[32];
  for (int i = 0; i < 32; i++) ir[i] = 500;
  int32_t red[32] = {0};
  spo2_hr_result_t res;
  hs_status_t st = spo2_hr_compute(ir, red, 32, 100, &res);
  TEST_ASSERT_EQUAL(HS_UNSTABLE, st);
}

void test_returns_unstable_when_ir_signal_is_flat(void) {
  int32_t ir[32];
  int32_t red[32];
  for (int i = 0; i < 32; i++) {
    ir[i] = 500;
    red[i] = 500;
  }
  spo2_hr_result_t res;
  hs_status_t st = spo2_hr_compute(ir, red, 32, 100, &res);
  TEST_ASSERT_EQUAL(HS_UNSTABLE, st);
}

void test_returns_unstable_when_only_one_peak_detected(void) {
  int32_t ir[32];
  int32_t red[32];
  for (int i = 0; i < 32; i++) {
    ir[i] = (i < 16) ? 100 : 200;
    red[i] = 500;
  }
  spo2_hr_result_t res;
  hs_status_t st = spo2_hr_compute(ir, red, 32, 100, &res);
  TEST_ASSERT_EQUAL(HS_UNSTABLE, st);
}

void test_returns_ok_for_periodic_pulse_signal(void) {
  int32_t ir[200];
  int32_t red[200];
  for (int i = 0; i < 200; i++) {
    int j = i % 80;
    ir[i] = (j < 10) ? 300 : 100;
    red[i] = (j < 10) ? 366 : 200;
  }
  spo2_hr_result_t res;
  hs_status_t st = spo2_hr_compute(ir, red, 200, 100, &res);
  printf("%d", res.spo2_x10);
  TEST_ASSERT_EQUAL(HS_OK, st);
  TEST_ASSERT_EQUAL(1, res.valid);
  TEST_ASSERT_EQUAL(75, res.heart_rate_bpm);
  TEST_ASSERT_TRUE(res.spo2_x10 >= 700 && res.spo2_x10 <= 1000);
}