/**
 * @file    ui_flow.h
 * @brief   OLED + 键盘的多级菜单导航引擎对外接口。
 * @owner   刘晏铭
 *
 * 与主状态机解耦：ui_flow 负责「显示什么 + 读什么键 + 菜单内部导航」，
 * 真正需要驱动传感器/上报/存储的动作以 ui_tick_result_t 交还给
 * app_statemachine 执行，状态迁移仍由后者决定。
 */
#ifndef APP_UI_FLOW_H
#define APP_UI_FLOW_H

#include "health_if.h"

typedef enum {
  UI_ACT_NONE = 0, /* 纯导航/重绘，app 无需做任何事 */
  UI_ACT_MEASURE,  /* 采集 items[0..item_count-1] 对应的传感器 */
  UI_ACT_UPLOAD,   /* 用当前 g_current_record 完成评分+BLE+存储 */
} ui_action_t;

typedef struct {
  ui_action_t action;
  hs_item_t items[2];
  uint8_t item_count;
} ui_tick_result_t;

/** @brief ST_MENU 态每 tick 调用一次：读键、走内部导航、按需重绘；
 *  返回本次需要 app 状态机执行的动作（多数 tick 返回 UI_ACT_NONE）。 */
ui_tick_result_t ui_tick(void);

/** @brief 一次 Upload 流程结束后由状态机调用，把导航复位回根菜单。 */
void ui_reset_to_root(void);

/** @brief 外部动作改变了 g_current_record 后，要求下一 tick 刷新当前页。 */
void ui_request_redraw(void);

#endif /* APP_UI_FLOW_H */
