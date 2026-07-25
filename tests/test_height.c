#include <stddef.h>

#include "height.h"
#include "test_util.h"

int main(void) {
  int32_t height = 0;
  int32_t distance = 0;

  /* 20.00°C、约 5.831ms 往返时间，对应约 1m 距离。 */
  CHECK_EQ(height_from_echo_us(5831U, 2000, 2000, &height, &distance), HS_OK);
  CHECK_NEAR(distance, 1001, 1);
  CHECK_NEAR(height, 999, 1);

  /* 同一回波宽度随温度升高应换算为更大的距离、更小的身高。 */
  int32_t height_cold = 0;
  int32_t height_hot = 0;
  CHECK_EQ(height_from_echo_us(5831U, 0, 2000, &height_cold, NULL), HS_OK);
  CHECK_EQ(height_from_echo_us(5831U, 4000, 2000, &height_hot, NULL), HS_OK);
  CHECK(height_cold > height_hot);

  CHECK_EQ(height_from_echo_us(0U, 2000, 2000, &height, NULL), HS_TIMEOUT);
  CHECK_EQ(height_from_echo_us(40000U, 2000, 2000, &height, NULL), HS_UNSTABLE);
  CHECK_EQ(height_from_echo_us(5831U, 20000, 2000, &height, NULL), HS_UNSTABLE);
  CHECK_EQ(height_from_echo_us(5831U, 2000, 0, &height, NULL), HS_UNSTABLE);
  CHECK_EQ(height_from_echo_us(11000U, 2000, 2000, &height, NULL), HS_UNSTABLE);

  return TEST_SUMMARY();
}
