/**
 * @file    board.h
 * @brief   板级引脚与资源定义（整机最终分配，与 docs/引脚分配表.md 严格一致）。
 * @owner   王宇浩（组长）
 *
 * 三人接线一律以本文件 + 引脚分配表为准。改动引脚必须同步二者并全员周知。
 * 引脚分配理由（FT 校验、EXTI/复用核对）见 docs/引脚分配表.md。
 */
#ifndef BOARD_H
#define BOARD_H

#include "stm32f1xx_hal.h"

/* ---- 固件版本号（HELLO 帧与调试串口打印用）---- */
#define FW_VERSION_MAJOR 0
#define FW_VERSION_MINOR 1
#define FW_VERSION_STR   "health-terminal v0.1"

/* ---- 状态/心跳 LED（板载，PC13，低电平点亮）---- */
#define LED_STATUS_PORT   GPIOC
#define LED_STATUS_PIN    GPIO_PIN_13
#define LED_STATUS_ON()   HAL_GPIO_WritePin(LED_STATUS_PORT, LED_STATUS_PIN, GPIO_PIN_RESET)
#define LED_STATUS_OFF()  HAL_GPIO_WritePin(LED_STATUS_PORT, LED_STATUS_PIN, GPIO_PIN_SET)
#define LED_STATUS_TOGGLE() HAL_GPIO_TogglePin(LED_STATUS_PORT, LED_STATUS_PIN)

/* ---- 调试串口 USART1：PA9(TX)/PA10(RX)，printf 重定向到此 ---- */
#define DEBUG_UART        USART1
#define DEBUG_UART_BAUD   115200

/* ---- BLE 透传 USART2：PA2(TX)/PA3(RX) ---- */
#define BLE_UART          USART2
#define BLE_UART_BAUD     9600

/* ---- I2C1 共享总线：PB6(SCL)/PB7(SDA) ---- */
#define SENSOR_I2C        I2C1
#define I2C_ADDR_OLED     (0x3C << 1)
#define I2C_ADDR_MPU6050  (0x68 << 1)
#define I2C_ADDR_MAX30102 (0x57 << 1)

/* ---- HX711#1 体重（查樊听）：SCK=PA1, DOUT=PB11 ---- */
#define HX711_W_SCK_PORT  GPIOA
#define HX711_W_SCK_PIN   GPIO_PIN_1
#define HX711_W_DOUT_PORT GPIOB
#define HX711_W_DOUT_PIN  GPIO_PIN_11

/* ---- HX711#2 握力（王宇浩）：SCK=PA0, DOUT=PB10 ---- */
#define HX711_G_SCK_PORT  GPIOA
#define HX711_G_SCK_PIN   GPIO_PIN_0
#define HX711_G_DOUT_PORT GPIOB
#define HX711_G_DOUT_PIN  GPIO_PIN_10

/* ---- HC-SR04 身高（查樊听）：Trig=PB9, Echo=PB8(FT, TIM4_CH3 输入捕获) ---- */
#define HCSR04_TRIG_PORT  GPIOB
#define HCSR04_TRIG_PIN   GPIO_PIN_9
#define HCSR04_ECHO_PORT  GPIOB
#define HCSR04_ECHO_PIN   GPIO_PIN_8

/* ---- DS18B20 温度（查樊听）：单总线 PB5(FT) ---- */
#define DS18B20_PORT      GPIOB
#define DS18B20_PIN       GPIO_PIN_5

/* ---- 4x4 矩阵键盘（刘晏铭）：行 PB12-15 输出，列 PA4-7 输入上拉 ---- */
#define KEYPAD_ROW_PORT   GPIOB
#define KEYPAD_ROW0_PIN   GPIO_PIN_12
#define KEYPAD_ROW1_PIN   GPIO_PIN_13
#define KEYPAD_ROW2_PIN   GPIO_PIN_14
#define KEYPAD_ROW3_PIN   GPIO_PIN_15
#define KEYPAD_COL_PORT   GPIOA
#define KEYPAD_COL0_PIN   GPIO_PIN_4
#define KEYPAD_COL1_PIN   GPIO_PIN_5
#define KEYPAD_COL2_PIN   GPIO_PIN_6
#define KEYPAD_COL3_PIN   GPIO_PIN_7

/* ---- 反应时间测试（刘晏铭）：独立 LED=PB1，独立按键=PB0(EXTI0) ---- */
#define REACTION_LED_PORT GPIOB
#define REACTION_LED_PIN  GPIO_PIN_1
#define REACTION_BTN_PORT GPIOB
#define REACTION_BTN_PIN  GPIO_PIN_0
#define REACTION_BTN_EXTI EXTI0_IRQn

#endif /* BOARD_H */
