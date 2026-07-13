/**
 * @file    keypad.c
 * @owner   刘晏铭
 * TODO(刘晏铭)：逐行拉低扫列，消抖，映射键值表。
 */
#include "keypad.h"

#if defined(MODULE_ENABLED_KEYPAD)
hs_status_t keypad_init(void) { return HS_NOT_IMPLEMENTED; }
char keypad_scan(void) { return 0; }
#else
hs_status_t keypad_init(void) { return HS_NOT_IMPLEMENTED; }
char keypad_scan(void) { return 0; }
#endif
