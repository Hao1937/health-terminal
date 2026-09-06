/**
 * @file  test_hx711_grip_demo.c
 * @brief 验证状态机反复 init 后，测力计假数据仍会逐次变化。
 */
#include "hx711_grip.h"
#include "test_util.h"

int main(void) {
  hs_sample_t first;
  hs_sample_t second;
  hs_sample_t third;

  CHECK_EQ(hx711_grip_init(), HS_OK);
  CHECK_EQ(hx711_grip_measure(&first), HS_OK);

  /* 模拟状态机下一轮测量前再次初始化。 */
  CHECK_EQ(hx711_grip_init(), HS_OK);
  CHECK_EQ(hx711_grip_measure(&second), HS_OK);

  CHECK_EQ(hx711_grip_init(), HS_OK);
  CHECK_EQ(hx711_grip_measure(&third), HS_OK);

  CHECK_EQ(first.primary, 323);
  CHECK_EQ(second.primary, 360);
  CHECK_EQ(third.primary, 397);
  CHECK(first.primary != second.primary);
  CHECK(second.primary != third.primary);
  CHECK_EQ(first.secondary, HS_VALUE_INVALID);
  CHECK_EQ(second.secondary, HS_VALUE_INVALID);
  CHECK_EQ(third.secondary, HS_VALUE_INVALID);

  return TEST_SUMMARY();
}
