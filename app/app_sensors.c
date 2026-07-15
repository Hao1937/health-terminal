/**
 * @file    app_sensors.c
 * @brief   传感器注册表：把测量项绑定到各模块实现与负责人。
 * @owner   王宇浩（组长）
 *
 * 状态机按本表遍历采集，不为每个传感器写分支。BODY(BMI/体脂率) 由 app 在拿到
 * 身高体重后用 algorithms 计算，不占传感器表项。
 */
#include "hcsr04.h"
#include "health_if.h"
#include "hx711_grip.h"
#include "hx711_weight.h"
#include "max30102.h"
#include "mpu6050.h"
#include "reaction.h"

const hs_sensor_t g_hs_registry[] = {
    {HS_ITEM_HEIGHT, "height", "查樊听", hcsr04_init, hcsr04_measure},
    {HS_ITEM_WEIGHT, "weight", "查樊听", hx711_weight_init,
     hx711_weight_measure},
    {HS_ITEM_HR_SPO2, "hr_spo2", "王宇浩（组长）", max30102_init,
     max30102_measure},
    {HS_ITEM_BALANCE, "balance", "刘晏铭", mpu6050_init, mpu6050_measure},
    {HS_ITEM_GRIP, "grip", "王宇浩（组长）", hx711_grip_init,
     hx711_grip_measure},
    {HS_ITEM_REACTION, "reaction", "刘晏铭", reaction_init, reaction_measure},
    /* 哨兵：measure==NULL 结尾 */
    {HS_ITEM_COUNT, 0, 0, 0, 0},
};

const char *hs_status_str(hs_status_t s) {
  switch (s) {
    case HS_OK:
      return "OK";
    case HS_NOT_READY:
      return "NOT_READY";
    case HS_UNSTABLE:
      return "UNSTABLE";
    case HS_TIMEOUT:
      return "TIMEOUT";
    case HS_NOT_IMPLEMENTED:
      return "NOT_IMPLEMENTED";
    default:
      return "UNKNOWN";
  }
}
