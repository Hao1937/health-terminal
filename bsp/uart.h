/**
 * @file    uart.h
 * @brief   两路串口：USART1 调试(printf)、USART2 BLE 透传。
 * @owner   王宇浩（组长）
 */
#ifndef BSP_UART_H
#define BSP_UART_H

#include <stddef.h>
#include <stdint.h>

#include "stm32f1xx_hal.h"

extern UART_HandleTypeDef g_debug_uart; /* USART1，printf 目标 */
extern UART_HandleTypeDef g_ble_uart;   /* USART2，BLE 模块 */

/** @brief 初始化两路串口。须在时钟配置后调用。 */
void uart_init(void);
/** @brief 向 BLE 串口阻塞发送一段字节（ble 模块用）。 */
void ble_uart_send(const uint8_t *data, size_t len);

#endif /* BSP_UART_H */
