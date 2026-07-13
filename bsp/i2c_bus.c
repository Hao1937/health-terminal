/**
 * @file    i2c_bus.c
 * @owner   王宇浩（组长）
 */
#include "i2c_bus.h"
#include "board.h"

I2C_HandleTypeDef g_sensor_i2c;

#define I2C_TIMEOUT_MS 100

void i2c_bus_init(void)
{
    g_sensor_i2c.Instance = SENSOR_I2C;
    g_sensor_i2c.Init.ClockSpeed = 100000;          /* 100kHz 标准模式，兼顾长线 */
    g_sensor_i2c.Init.DutyCycle = I2C_DUTYCYCLE_2;
    g_sensor_i2c.Init.OwnAddress1 = 0;
    g_sensor_i2c.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    g_sensor_i2c.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    g_sensor_i2c.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    g_sensor_i2c.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&g_sensor_i2c) != HAL_OK) {
        while (1) { }
    }
}

int i2c_mem_write(uint16_t dev_addr, uint16_t reg, const uint8_t *buf, uint16_t len)
{
    return (int)HAL_I2C_Mem_Write(&g_sensor_i2c, dev_addr, reg,
                                  I2C_MEMADD_SIZE_8BIT, (uint8_t *)buf, len, I2C_TIMEOUT_MS);
}

int i2c_mem_read(uint16_t dev_addr, uint16_t reg, uint8_t *buf, uint16_t len)
{
    return (int)HAL_I2C_Mem_Read(&g_sensor_i2c, dev_addr, reg,
                                 I2C_MEMADD_SIZE_8BIT, buf, len, I2C_TIMEOUT_MS);
}

int i2c_probe(uint16_t dev_addr)
{
    return (HAL_I2C_IsDeviceReady(&g_sensor_i2c, dev_addr, 2, I2C_TIMEOUT_MS) == HAL_OK) ? 1 : 0;
}
