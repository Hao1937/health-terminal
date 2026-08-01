/**
 * @file    ds18b20.c
 * @brief   DS18B20 三线供电模式驱动，DQ=PB5，外接 4.7k 上拉到 3.3V。
 * @owner   查樊听
 */
#include "ds18b20.h"

#if defined(MODULE_ENABLED_DS18B20)

#include "board.h"
#include "delay.h"
#include "stm32f1xx_hal.h"

#define DS18B20_CMD_SKIP_ROM 0xCCU
#define DS18B20_CMD_CONVERT_T 0x44U
#define DS18B20_CMD_READ_SCRATCHPAD 0xBEU
#define DS18B20_CONVERSION_TIMEOUT_MS 800U

static uint8_t s_initialized;

static void ow_drive_low(void) {
  HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
}

static void ow_release(void) {
  HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
}

static void irq_restore(uint32_t primask) {
  if (primask == 0U) {
    __enable_irq();
  }
}

static uint8_t ow_reset(void) {
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  ow_drive_low();
  delay_us(500U);
  ow_release();
  delay_us(70U);
  uint8_t present =
      (HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN) == GPIO_PIN_RESET);
  delay_us(410U);
  irq_restore(primask);
  return present;
}

static void ow_write_bit(uint8_t bit) {
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  ow_drive_low();
  if (bit != 0U) {
    delay_us(6U);
    ow_release();
    delay_us(64U);
  } else {
    delay_us(60U);
    ow_release();
    delay_us(10U);
  }
  irq_restore(primask);
}

static uint8_t ow_read_bit(void) {
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  ow_drive_low();
  delay_us(3U);
  ow_release();
  delay_us(10U);
  uint8_t bit = (HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN) == GPIO_PIN_SET);
  delay_us(53U);
  irq_restore(primask);
  return bit;
}

static void ow_write_byte(uint8_t value) {
  for (uint8_t i = 0U; i < 8U; ++i) {
    ow_write_bit(value & 0x01U);
    value >>= 1;
  }
}

static uint8_t ow_read_byte(void) {
  uint8_t value = 0U;
  for (uint8_t i = 0U; i < 8U; ++i) {
    value |= (uint8_t)(ow_read_bit() << i);
  }
  return value;
}

static uint8_t crc8_maxim(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0U;
  while (len-- != 0U) {
    uint8_t in = *data++;
    for (uint8_t i = 0U; i < 8U; ++i) {
      uint8_t mix = (crc ^ in) & 0x01U;
      crc >>= 1;
      if (mix != 0U) {
        crc ^= 0x8CU;
      }
      in >>= 1;
    }
  }
  return crc;
}

hs_status_t ds18b20_init(void) {
  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitTypeDef gpio = {0};
  gpio.Pin = DS18B20_PIN;
  gpio.Mode = GPIO_MODE_OUTPUT_OD;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(DS18B20_PORT, &gpio);
  ow_release();
  delay_us(5U);

  s_initialized = ow_reset();
  return s_initialized != 0U ? HS_OK : HS_TIMEOUT;
}

hs_status_t ds18b20_read_temperature(int32_t *temperature_x100) {
  if (temperature_x100 == NULL) {
    return HS_UNSTABLE;
  }
  if (s_initialized == 0U && ds18b20_init() != HS_OK) {
    return HS_TIMEOUT;
  }

  if (ow_reset() == 0U) {
    s_initialized = 0U;
    return HS_TIMEOUT;
  }
  ow_write_byte(DS18B20_CMD_SKIP_ROM);
  ow_write_byte(DS18B20_CMD_CONVERT_T);

  /* 三线供电模式下器件完成转换后会释放总线为高；12 bit 最长 750ms。 */
  uint32_t started = HAL_GetTick();
  while (ow_read_bit() == 0U) {
    if ((HAL_GetTick() - started) >= DS18B20_CONVERSION_TIMEOUT_MS) {
      return HS_TIMEOUT;
    }
    HAL_Delay(1U);
  }

  if (ow_reset() == 0U) {
    s_initialized = 0U;
    return HS_TIMEOUT;
  }
  ow_write_byte(DS18B20_CMD_SKIP_ROM);
  ow_write_byte(DS18B20_CMD_READ_SCRATCHPAD);

  uint8_t scratchpad[9];
  for (uint8_t i = 0U; i < sizeof(scratchpad); ++i) {
    scratchpad[i] = ow_read_byte();
  }
  if (crc8_maxim(scratchpad, 8U) != scratchpad[8]) {
    return HS_UNSTABLE;
  }

  int16_t raw = (int16_t)(((uint16_t)scratchpad[1] << 8) | scratchpad[0]);
  int32_t centi = ((int32_t)raw * 100) / 16;
  if (centi < -5500 || centi > 12500) {
    return HS_UNSTABLE;
  }

  *temperature_x100 = centi;
  return HS_OK;
}

hs_status_t ds18b20_measure(hs_sample_t *out) {
  if (out == NULL) {
    return HS_UNSTABLE;
  }
  hs_status_t status = ds18b20_read_temperature(&out->primary);
  out->secondary = HS_VALUE_INVALID;
  return status;
}

#else

hs_status_t ds18b20_init(void) { return HS_NOT_IMPLEMENTED; }
hs_status_t ds18b20_measure(hs_sample_t *out) {
  (void)out;
  return HS_NOT_IMPLEMENTED;
}
hs_status_t ds18b20_read_temperature(int32_t *temperature_x100) {
  (void)temperature_x100;
  return HS_NOT_IMPLEMENTED;
}

#endif
