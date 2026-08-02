/**
 * @file    mpu6050.c
 * @owner   刘晏铭
 *
 * MPU6050 平衡晃动指数：轮询 I2C1 读取加速度/陀螺，互补滤波得到
 * pitch/roll 序列，再用 balance_index() 输出 balance_x10。当前 app 每次
 * 只调用一次 measure()，因此采样窗口在本函数内部一次完成。
 */
#include "mpu6050.h"

#if defined(MODULE_ENABLED_MPU6050)

#include <stdint.h>
#include <stdio.h>

#include "balance.h"
#include "board.h"
#include "delay.h"
#include "filters.h"
#include "i2c_bus.h"
#include "oled.h"
#include "tick.h"

#define MPU_REG_SMPLRT_DIV 0x19u
#define MPU_REG_CONFIG 0x1Au
#define MPU_REG_GYRO_CONFIG 0x1Bu
#define MPU_REG_ACCEL_CONFIG 0x1Cu
#define MPU_REG_INT_STATUS 0x3Au
#define MPU_REG_ACCEL_XOUT_H 0x3Bu
#define MPU_REG_PWR_MGMT_1 0x6Bu
#define MPU_REG_WHO_AM_I 0x75u

#define MPU_WHO_AM_I_VALUE 0x68u
#define MPU_SAMPLE_COUNT 250u
#define MPU_SAMPLE_PERIOD_MS 20u
#define MPU_PROGRESS_STEP 25u
#define MPU_FRAME_TIMEOUT_MS 60u
#define MPU_INIT_TIMEOUT_MS 120u
#define MPU_COMP_ALPHA 0.98f
#define MPU_GYRO_LSB_PER_DPS 131.0f
#define MPU_DEG_PER_RAD_X100 5729 /* 180/pi * 100 */

static comp_filter_t s_pitch_filter;
static comp_filter_t s_roll_filter;
static int32_t s_pitch_x100[MPU_SAMPLE_COUNT];
static int32_t s_roll_x100[MPU_SAMPLE_COUNT];
static uint8_t s_ready;

static hs_status_t mpu_write_u8(uint8_t reg, uint8_t val) {
  return (i2c_mem_write(I2C_ADDR_MPU6050, reg, &val, 1) == 0) ? HS_OK
                                                              : HS_TIMEOUT;
}

static hs_status_t mpu_read_u8(uint8_t reg, uint8_t *val) {
  if (val == 0) return HS_NOT_READY;
  return (i2c_mem_read(I2C_ADDR_MPU6050, reg, val, 1) == 0) ? HS_OK
                                                            : HS_TIMEOUT;
}

static int16_t be_i16(const uint8_t *p) {
  return (int16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static int32_t isqrt64(int64_t v) {
  if (v <= 0) return 0;
  int64_t x = v;
  int64_t y = (x + 1) / 2;
  while (y < x) {
    x = y;
    y = (x + v / x) / 2;
  }
  return (int32_t)x;
}

/* atan(z) 近似，输入/输出均为 rad×10000。用于避免在 F103 上链接 libm。 */
static int32_t atan_approx_rad_x10000(int32_t z_x10000) {
  int sign = 1;
  if (z_x10000 < 0) {
    sign = -1;
    z_x10000 = -z_x10000;
  }

  const int32_t pi_2 = 15708; /* pi/2 ×10000 */
  int invert = 0;
  if (z_x10000 > 10000) {
    int64_t z = z_x10000;
    z_x10000 = (int32_t)(100000000LL / z);
    invert = 1;
  }

  int64_t z = z_x10000;
  int64_t z2 = (z * z) / 10000;
  /* atan(x) ~= x*(pi/4 + 0.273*(1-|x|)), x in [0,1] */
  int64_t factor = 7854 + (2730 * (10000 - z_x10000)) / 10000;
  int32_t a = (int32_t)((z * factor) / 10000);
  (void)z2; /* 保留变量名含义，避免误以为这里需要二次项。 */

  if (invert) a = pi_2 - a;
  return sign * a;
}

static int32_t atan2_approx_rad_x10000(int32_t y, int32_t x) {
  const int32_t pi = 31416;
  const int32_t pi_2 = 15708;

  if (x == 0) {
    if (y > 0) return pi_2;
    if (y < 0) return -pi_2;
    return 0;
  }

  int32_t z = (int32_t)(((int64_t)y * 10000) / x);
  int32_t a = atan_approx_rad_x10000(z);
  if (x > 0) return a;
  return (y >= 0) ? (a + pi) : (a - pi);
}

static int32_t rad_x10000_to_deg_x100(int32_t rad_x10000) {
  return (int32_t)(((int64_t)rad_x10000 * MPU_DEG_PER_RAD_X100) / 10000);
}

static int32_t accel_pitch_x100(int16_t ax, int16_t ay, int16_t az) {
  int64_t denom_sq = (int64_t)ay * ay + (int64_t)az * az;
  int32_t denom = isqrt64(denom_sq);
  int32_t rad = atan2_approx_rad_x10000(ax, denom);
  return rad_x10000_to_deg_x100(rad);
}

static int32_t accel_roll_x100(int16_t ay, int16_t az) {
  int32_t rad = atan2_approx_rad_x10000(ay, az);
  return rad_x10000_to_deg_x100(rad);
}

static int32_t round_deg_to_x100(float deg) {
  float scaled = deg * 100.0f;
  return (int32_t)(scaled >= 0.0f ? (scaled + 0.5f) : (scaled - 0.5f));
}

static hs_status_t wait_data_ready(void) {
  uint32_t t0 = tick_now_ms();
  do {
    uint8_t st = 0;
    if (mpu_read_u8(MPU_REG_INT_STATUS, &st) != HS_OK) return HS_TIMEOUT;
    if (st & 0x01u) return HS_OK;
  } while (tick_now_ms() - t0 < MPU_FRAME_TIMEOUT_MS);
  return HS_TIMEOUT;
}

static void balance_show_countdown(void) {
  oled_clear();
  oled_show_text(0, 0, "BALANCE TEST");
  oled_show_text(0, 2, "FIX ON WAIST");
  oled_show_text(0, 3, "KEEP STILL");
  for (int n = 3; n >= 1; --n) {
    char line[24];
    snprintf(line, sizeof(line), "READY %d", n);
    oled_show_text(0, 5, "                     ");
    oled_show_text(0, 5, line);
    delay_ms(1000);
  }
}

static void balance_show_progress(uint8_t done, uint8_t total) {
  char line[24];
  uint8_t pct = (uint8_t)(((uint16_t)done * 100u) / total);
  oled_clear();
  oled_show_text(0, 0, "TESTING BALANCE");
  snprintf(line, sizeof(line), "PROGRESS:%u%%", (unsigned)pct);
  oled_show_text(0, 2, line);
  uint8_t bars = (uint8_t)(pct / 5u);
  char bar[22];
  for (uint8_t i = 0; i < 20u; ++i) bar[i] = (i < bars) ? 'O' : '-';
  bar[20] = '\0';
  oled_show_text(0, 4, bar);
}

static void balance_show_result(int32_t idx_x10) {
  char line[24];
  oled_clear();
  oled_show_text(0, 0, "BALANCE RESULT");
  snprintf(line, sizeof(line), "BAL:%ld.%ld", (long)(idx_x10 / 10),
           (long)(idx_x10 % 10));
  oled_show_text(0, 2, line);
  if (idx_x10 <= 30) {
    oled_show_text(0, 4, "GOOD");
  } else if (idx_x10 <= 60) {
    oled_show_text(0, 4, "NORMAL");
  } else if (idx_x10 <= 100) {
    oled_show_text(0, 4, "WEAK");
  } else {
    oled_show_text(0, 4, "RISK");
  }
}

static void balance_show_error(const char *msg) {
  oled_clear();
  oled_show_text(0, 0, "BALANCE ERROR");
  oled_show_text(0, 2, msg);
}

hs_status_t mpu6050_init(void) {
  if (!i2c_probe(I2C_ADDR_MPU6050)) {
    s_ready = 0;
    return HS_TIMEOUT;
  }

  uint8_t who = 0;
  if (mpu_read_u8(MPU_REG_WHO_AM_I, &who) != HS_OK ||
      who != MPU_WHO_AM_I_VALUE) {
    s_ready = 0;
    return HS_TIMEOUT;
  }

  if (mpu_write_u8(MPU_REG_PWR_MGMT_1, 0x80u) != HS_OK) {
    s_ready = 0;
    return HS_TIMEOUT;
  }
  delay_ms(50);

  uint32_t t0 = tick_now_ms();
  uint8_t reset_done = 0;
  do {
    uint8_t pwr = 0;
    if (mpu_read_u8(MPU_REG_PWR_MGMT_1, &pwr) != HS_OK) {
      s_ready = 0;
      return HS_TIMEOUT;
    }
    if ((pwr & 0x80u) == 0) {
      reset_done = 1;
      break;
    }
    delay_ms(1);
  } while (tick_now_ms() - t0 < MPU_INIT_TIMEOUT_MS);
  if (!reset_done) {
    s_ready = 0;
    return HS_TIMEOUT;
  }

  if (mpu_write_u8(MPU_REG_PWR_MGMT_1, 0x01u) != HS_OK ||
      mpu_write_u8(MPU_REG_CONFIG, 0x03u) != HS_OK ||
      mpu_write_u8(MPU_REG_SMPLRT_DIV, 19u) != HS_OK ||
      mpu_write_u8(MPU_REG_GYRO_CONFIG, 0x00u) != HS_OK ||
      mpu_write_u8(MPU_REG_ACCEL_CONFIG, 0x00u) != HS_OK) {
    s_ready = 0;
    return HS_TIMEOUT;
  }
  delay_ms(10);

  comp_init(&s_pitch_filter, MPU_COMP_ALPHA);
  comp_init(&s_roll_filter, MPU_COMP_ALPHA);
  s_ready = 1;
  return HS_OK;
}

hs_status_t mpu6050_measure(hs_sample_t *out) {
  if (out == 0) return HS_NOT_READY;
  out->primary = HS_VALUE_INVALID;
  out->secondary = HS_VALUE_INVALID;

  if (!s_ready) {
    hs_status_t st = mpu6050_init();
    if (st != HS_OK) return st;
  }
  comp_init(&s_pitch_filter, MPU_COMP_ALPHA);
  comp_init(&s_roll_filter, MPU_COMP_ALPHA);

  balance_show_countdown();
  balance_show_progress(0, MPU_SAMPLE_COUNT);

  uint32_t last_tick = tick_now_ms();
  for (uint16_t i = 0; i < MPU_SAMPLE_COUNT; ++i) {
    uint32_t target = last_tick + MPU_SAMPLE_PERIOD_MS;
    while ((int32_t)(tick_now_ms() - target) < 0) {
      /* 保持约 50Hz 采样；裸机测量窗口内允许短暂阻塞 UI。 */
    }
    uint32_t now = tick_now_ms();
    float dt = (float)(now - last_tick) / 1000.0f;
    if (dt < 0.005f || dt > 0.050f) dt = 0.020f;
    last_tick = now;

    hs_status_t st = wait_data_ready();
    if (st != HS_OK) {
      balance_show_error("MPU TIMEOUT");
      delay_ms(1000);
      return st;
    }

    uint8_t raw[14];
    if (i2c_mem_read(I2C_ADDR_MPU6050, MPU_REG_ACCEL_XOUT_H, raw,
                     sizeof(raw)) != 0) {
      s_ready = 0;
      balance_show_error("I2C READ FAIL");
      delay_ms(1000);
      return HS_TIMEOUT;
    }

    int16_t ax = be_i16(&raw[0]);
    int16_t ay = be_i16(&raw[2]);
    int16_t az = be_i16(&raw[4]);
    int16_t gx = be_i16(&raw[8]);
    int16_t gy = be_i16(&raw[10]);

    int32_t pitch_acc_x100 = accel_pitch_x100(ax, ay, az);
    int32_t roll_acc_x100 = accel_roll_x100(ay, az);
    float pitch = comp_update(&s_pitch_filter, (float)pitch_acc_x100 / 100.0f,
                              (float)gx / MPU_GYRO_LSB_PER_DPS, dt);
    float roll = comp_update(&s_roll_filter, (float)roll_acc_x100 / 100.0f,
                             (float)gy / MPU_GYRO_LSB_PER_DPS, dt);
    s_pitch_x100[i] = round_deg_to_x100(pitch);
    s_roll_x100[i] = round_deg_to_x100(roll);

    if (((i + 1u) % MPU_PROGRESS_STEP) == 0u || i + 1u == MPU_SAMPLE_COUNT) {
      balance_show_progress((uint8_t)(i + 1u), MPU_SAMPLE_COUNT);
    }
  }

  int32_t idx_x10 = HS_VALUE_INVALID;
  hs_status_t st =
      balance_index(s_pitch_x100, s_roll_x100, MPU_SAMPLE_COUNT, &idx_x10);
  if (st != HS_OK) {
    balance_show_error("SAMPLE ERROR");
    delay_ms(1000);
    return st;
  }

  out->primary = idx_x10;
  balance_show_result(idx_x10);
  delay_ms(1500);
  return HS_OK;
}

#else /* 本板 MODULE_SET 未包含该模块：桩替代，仅供 app 注册表链接 */

hs_status_t mpu6050_init(void) { return HS_NOT_IMPLEMENTED; }
hs_status_t mpu6050_measure(hs_sample_t *out) {
  (void)out;
  return HS_NOT_IMPLEMENTED;
}

#endif
