/**
 * @file    grip_demo.c
 * @owner   王宇浩（组长）
 */
#include "grip_demo.h"

int32_t grip_demo_value(uint32_t index) {
  /* 演示数据：约 32kg，用于没有接入 HX711 时验证整机链路。 */
  static const int32_t values_kg_x10[] = {318, 321, 324, 319, 322};
  return values_kg_x10[index %
                       (sizeof(values_kg_x10) / sizeof(values_kg_x10[0]))];
}
