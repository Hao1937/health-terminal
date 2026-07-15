/**
 * @file    app.h
 * @brief   主状态机与 UI 流程对外接口。
 * @owner   王宇浩（组长）
 */
#ifndef APP_H
#define APP_H

#include "health_record.h"

/* 主状态机状态 */
typedef enum {
  ST_BOOT = 0, /* 上电自检：探测 I2C 从机、打印版本 */
  ST_MENU,     /* 主菜单：键盘选择测量项 */
  ST_MEASURE,  /* 正在测量选中项 */
  ST_RESULT,   /* 展示单项/整机结果与综合评分 */
  ST_HISTORY,  /* 浏览历史记录 */
} app_state_t;

/** @brief 初始化时钟/外设/模块，进入 BOOT。 */
void app_init(void);
/** @brief 主循环调用一次：推进状态机（裸机，无 RTOS）。 */
void app_tick(void);

/** @brief 当前整机采集缓存（供 UI/BLE/存储共享）。 */
extern measurement_record_t g_current_record;

#endif /* APP_H */
