/**
 * @file    grip_demo.c
 * @owner   王宇浩（组长）
 */
#include "grip_demo.h"

int32_t grip_demo_value(uint32_t index) {
  /*
   * 30.0~40.0kg 的确定性伪随机序列，0.1kg 为一步。
   * 37 与 101 互质，因此一个周期内会覆盖全部 101 个可选值；先取模可
   * 避免 index 很大时乘法溢出。确定性保证主机测试和整机演示可复现。
   */
  return 300 + (int32_t)(((index % 101U) * 37U + 23U) % 101U);
}
