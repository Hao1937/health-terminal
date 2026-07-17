#include <stdint.h>

#include "health_if.h"
#include "max30102.h"
#include "mock_delay.h"
#include "mock_i2c_bus.h"
#include "mock_tick.h"
#include "spo2_hr.h" /* 真实实现，不 mock：纯算法，参与链接即可 */
#include "unity.h"
#include "unity_internals.h"

void setUp(void) {}
void tearDown(void) {}

#define REG_PART_ID 0xFF
#define MAX30102_ADDR (0x57 << 1)
#define REG_MODE_CONFIG 0x09
#define PART_ID_MAX30102 0x15
#define MODE_RESET 0x40
#define REG_FIFO_WR_PTR 0x04
#define REG_OVF_COUNTER 0x05
#define REG_FIFO_RD_PTR 0x06
#define REG_FIFO_CONFIG 0x08
#define REG_SPO2_CONFIG 0x0A
#define REG_LED1_PA 0x0C
#define REG_LED2_PA 0x0D
void test_init_i2c_read_error_returns_timeout(void) {
  i2c_mem_read_ExpectAndReturn(MAX30102_ADDR, REG_PART_ID, NULL, 1, -1);
  i2c_mem_read_IgnoreArg_buf();
  TEST_ASSERT_EQUAL(HS_TIMEOUT, max30102_init());
}
void test_init_wrong_part_id_returns_timeout(void) {
  uint8_t id_wrong = 0x00;
  i2c_mem_read_ExpectAndReturn(MAX30102_ADDR, REG_PART_ID, NULL, 1, 0);
  i2c_mem_read_IgnoreArg_buf();
  i2c_mem_read_ReturnThruPtr_buf(&id_wrong);

  TEST_ASSERT_EQUAL(HS_TIMEOUT, max30102_init());
}
void test_init_reset_bit_stuck_times_out(void) {
  uint8_t id_ok = PART_ID_MAX30102;
  uint8_t mode_stuck = MODE_RESET;

  i2c_mem_read_ExpectAndReturn(MAX30102_ADDR, REG_PART_ID, NULL, 1, 0);
  i2c_mem_read_IgnoreArg_buf();
  i2c_mem_read_ReturnThruPtr_buf(&id_ok);

  i2c_mem_write_ExpectAndReturn(MAX30102_ADDR, REG_MODE_CONFIG, NULL, 1, 0);
  i2c_mem_write_IgnoreArg_buf();

  tick_now_ms_ExpectAndReturn(0);

  i2c_mem_read_ExpectAndReturn(MAX30102_ADDR, REG_MODE_CONFIG, NULL, 1, 0);
  i2c_mem_read_IgnoreArg_buf();
  i2c_mem_read_ReturnThruPtr_buf(&mode_stuck);

  tick_now_ms_ExpectAndReturn(101);

  TEST_ASSERT_EQUAL(HS_TIMEOUT, max30102_init());
}

void test_init_happy_path_writes_expected_registers(void) {
  uint8_t id_ok = PART_ID_MAX30102;
  uint8_t mode_clean = 0x00;
  i2c_mem_read_ExpectAndReturn(MAX30102_ADDR, REG_PART_ID, NULL, 1, 0);
  i2c_mem_read_IgnoreArg_buf();
  i2c_mem_read_ReturnThruPtr_buf(&id_ok);

  i2c_mem_write_ExpectAndReturn(MAX30102_ADDR, REG_MODE_CONFIG, NULL, 1, 0);
  i2c_mem_write_IgnoreArg_buf();

  tick_now_ms_ExpectAndReturn(0);
  i2c_mem_read_ExpectAndReturn(MAX30102_ADDR, REG_MODE_CONFIG, NULL, 1, 0);
  i2c_mem_read_IgnoreArg_buf();
  i2c_mem_read_ReturnThruPtr_buf(&mode_clean);
  tick_now_ms_ExpectAndReturn(1);

  i2c_mem_write_ExpectAndReturn(MAX30102_ADDR, REG_FIFO_WR_PTR, NULL, 1, 0);
  i2c_mem_write_IgnoreArg_buf();
  i2c_mem_write_ExpectAndReturn(MAX30102_ADDR, REG_OVF_COUNTER, NULL, 1, 0);
  i2c_mem_write_IgnoreArg_buf();
  i2c_mem_write_ExpectAndReturn(MAX30102_ADDR, REG_FIFO_RD_PTR, NULL, 1, 0);
  i2c_mem_write_IgnoreArg_buf();

  i2c_mem_write_ExpectAndReturn(MAX30102_ADDR, REG_FIFO_CONFIG, NULL, 1, 0);
  i2c_mem_write_IgnoreArg_buf();
  i2c_mem_write_ExpectAndReturn(MAX30102_ADDR, REG_MODE_CONFIG, NULL, 1, 0);
  i2c_mem_write_IgnoreArg_buf();
  i2c_mem_write_ExpectAndReturn(MAX30102_ADDR, REG_SPO2_CONFIG, NULL, 1, 0);
  i2c_mem_write_IgnoreArg_buf();
  i2c_mem_write_ExpectAndReturn(MAX30102_ADDR, REG_LED1_PA, NULL, 1, 0);
  i2c_mem_write_IgnoreArg_buf();
  i2c_mem_write_ExpectAndReturn(MAX30102_ADDR, REG_LED2_PA, NULL, 1, 0);
  i2c_mem_write_IgnoreArg_buf();

  i2c_mem_write_ExpectAndReturn(MAX30102_ADDR, REG_FIFO_WR_PTR, NULL, 1, 0);
  i2c_mem_write_IgnoreArg_buf();
  i2c_mem_write_ExpectAndReturn(MAX30102_ADDR, REG_OVF_COUNTER, NULL, 1, 0);
  i2c_mem_write_IgnoreArg_buf();
  i2c_mem_write_ExpectAndReturn(MAX30102_ADDR, REG_FIFO_RD_PTR, NULL, 1, 0);
  i2c_mem_write_IgnoreArg_buf();

  TEST_ASSERT_EQUAL(HS_OK, max30102_init());
}