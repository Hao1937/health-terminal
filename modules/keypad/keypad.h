/**
 * @file    keypad.h
 * @brief   4x4 矩阵键盘（行 PB12-15 输出，列 PA4-7 输入上拉）。
 * @owner   刘晏铭
 */
#ifndef MODULE_KEYPAD_H
#define MODULE_KEYPAD_H

#include "health_if.h"

hs_status_t keypad_init(void);
/** @brief 扫描一次，返回按键字符('0'-'9''A'-'D''*''#')，无按键返回 0。 */
char keypad_scan(void);

#endif /* MODULE_KEYPAD_H */
