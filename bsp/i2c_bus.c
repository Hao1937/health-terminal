/**
 * @file    i2c_bus.c
 * @owner   王宇浩（组长）
 */
#include "i2c_bus.h"

#include "board.h"
#include "delay.h"

I2C_HandleTypeDef g_sensor_i2c;

#define I2C_TIMEOUT_MS 100

void i2c_bus_recover(void) {
  GPIO_InitTypeDef g = {0};

  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_I2C1_CLK_ENABLE();
  __HAL_I2C_DISABLE(&g_sensor_i2c);

  /* 临时切成开漏 GPIO：写 1 是释放总线，外部上拉负责拉高。 */
  g.Pin = GPIO_PIN_6 | GPIO_PIN_7;
  g.Mode = GPIO_MODE_OUTPUT_OD;
  g.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &g);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_SET);
  delay_us(5);

  /* 从机若停在接收状态，最多 9 个时钟可以把它推进到释放 SDA。 */
  for (uint8_t i = 0; i < 9 &&
                      HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7) == GPIO_PIN_RESET;
       ++i) {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
    delay_us(5);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
    delay_us(5);
  }

  /* 生成 STOP：SDA 低 -> SCL 高 -> SDA 高。 */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
  delay_us(5);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
  delay_us(5);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
  delay_us(5);

  /* 交还给 I2C 外设的复用开漏模式；调用方随后执行 i2c_bus_init()。 */
  g.Pin = GPIO_PIN_6 | GPIO_PIN_7;
  g.Mode = GPIO_MODE_AF_OD;
  HAL_GPIO_Init(GPIOB, &g);
}

void i2c_bus_init(void) {
  g_sensor_i2c.Instance = SENSOR_I2C;
  g_sensor_i2c.Init.ClockSpeed = 100000; /* 100kHz 标准模式，兼顾长线 */
  g_sensor_i2c.Init.DutyCycle = I2C_DUTYCYCLE_2;
  g_sensor_i2c.Init.OwnAddress1 = 0;
  g_sensor_i2c.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  g_sensor_i2c.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  g_sensor_i2c.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  g_sensor_i2c.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&g_sensor_i2c) != HAL_OK) {
    while (1) {
    }
  }
}

int i2c_mem_write(uint16_t dev_addr, uint16_t reg, const uint8_t *buf,
                  uint16_t len) {
  return (int)HAL_I2C_Mem_Write(&g_sensor_i2c, dev_addr, reg,
                                I2C_MEMADD_SIZE_8BIT, (uint8_t *)buf, len,
                                I2C_TIMEOUT_MS);
}

int i2c_mem_read(uint16_t dev_addr, uint16_t reg, uint8_t *buf, uint16_t len) {
  return (int)HAL_I2C_Mem_Read(&g_sensor_i2c, dev_addr, reg,
                               I2C_MEMADD_SIZE_8BIT, buf, len, I2C_TIMEOUT_MS);
}

int i2c_probe(uint16_t dev_addr) {
  return (HAL_I2C_IsDeviceReady(&g_sensor_i2c, dev_addr, 2, I2C_TIMEOUT_MS) ==
          HAL_OK)
             ? 1
             : 0;
}
