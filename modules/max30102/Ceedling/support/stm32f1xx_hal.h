#ifndef STM32F1XX_HAL_H
#define STM32F1XX_HAL_H
/* host 测试专用桩：只提供 board.h/i2c_bus.h 语法层面需要的类型占位，
 * 内容不重要——max30102.c 从不直接访问 I2C_HandleTypeDef 内部字段，
 * i2c_mem_read/i2c_mem_write 在测试里整个被 CMock 替换，不会真的用到它。 */
typedef struct {
  int dummy;
} I2C_HandleTypeDef;
#endif