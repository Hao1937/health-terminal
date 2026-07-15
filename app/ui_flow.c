/**
 * @file    ui_flow.c
 * @brief   OLED + 键盘的菜单交互流程（骨架，待 OLED/键盘模块就绪后填充）。
 * @owner   刘晏铭
 *
 * 与主状态机解耦：ui_flow 负责「显示什么 + 读什么键」，状态迁移仍由
 * app_statemachine 决定。当前为占位，保证整机可编译。
 */
#include "app.h"
#include "keypad.h"
#include "oled.h"

/* 菜单项顺序与展示文案，后续 OLED 就绪后渲染 */
static const char *const s_menu_items[] = {
    "1.Height/Weight", "2.HR/SpO2",      "3.Balance", "4.Grip",
    "5.Reaction",      "6.Score/Upload", "7.History",
};

/** @brief 渲染主菜单（TODO 刘晏铭：分页+高亮选中项）。 */
void ui_render_menu(uint8_t selected) {
  (void)selected;
  oled_clear();
  for (uint8_t i = 0;
       i < (uint8_t)(sizeof(s_menu_items) / sizeof(s_menu_items[0])); ++i) {
    oled_show_text(0, i, s_menu_items[i]);
  }
}

/** @brief 读一次键盘，返回菜单选择（0 表示无输入）。 */
char ui_poll_key(void) { return keypad_scan(); }
