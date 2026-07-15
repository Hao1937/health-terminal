/**
 * @file    uart.c
 * @owner   王宇浩（组长）
 */
#include "uart.h"

#include "board.h"

UART_HandleTypeDef g_debug_uart;
UART_HandleTypeDef g_ble_uart;

static void uart_config(UART_HandleTypeDef *h, USART_TypeDef *inst,
                        uint32_t baud) {
  h->Instance = inst;
  h->Init.BaudRate = baud;
  h->Init.WordLength = UART_WORDLENGTH_8B;
  h->Init.StopBits = UART_STOPBITS_1;
  h->Init.Parity = UART_PARITY_NONE;
  h->Init.Mode = UART_MODE_TX_RX;
  h->Init.HwFlowCtl = UART_HWCONTROL_NONE;
  h->Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(h) != HAL_OK) {
    while (1) {
    }
  }
}

void uart_init(void) {
  uart_config(&g_debug_uart, DEBUG_UART, DEBUG_UART_BAUD);
  uart_config(&g_ble_uart, BLE_UART, BLE_UART_BAUD);
}

void ble_uart_send(const uint8_t *data, size_t len) {
  HAL_UART_Transmit(&g_ble_uart, (uint8_t *)data, (uint16_t)len, HAL_MAX_DELAY);
}
