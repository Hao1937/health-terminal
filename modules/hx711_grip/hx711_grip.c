/**
 * @file    hx711_grip.c
 * @owner   王宇浩（组长）
 *
 * 握力：HX711#2 + 悬臂梁测力传感器（如
 * YZC-131）。测量语义是「一次握持的最大力」， 因此 measure()
 * 在一个时间窗内连续采样、取净读数峰值，再按标定折算成 kg×10。 底层 24bit
 * 时序复用 bsp/hx711（体重模块也用同一驱动）。
 */
#include "hx711_grip.h"

#if defined(MODULE_ENABLED_HX711_GRIP)

#include "grip_demo.h"

#if !defined(GRIP_DEMO_MODE)
#include "board.h"
#include "hx711.h"
#include "stm32f1xx_hal.h"
#endif

#if defined(GRIP_DEMO_MODE)

/*
 * 假数据模式：默认用 30.0~40.0kg 的软件数据验证握力数据从模块接口进入
 * 状态机、记录、OLED、BLE 与综合评分的完整链路。
 * 这里返回的是明确标记的确定性软件数据，不是 HX711 实测数据。
 */
static uint32_t s_demo_index;

/* 状态机每次测量前都会调用 init；不要在这里重置序号，否则永远只输出首值。 */
hs_status_t hx711_grip_init(void) { return HS_OK; }

hs_status_t hx711_grip_measure(hs_sample_t *out) {
  if (out == 0) return HS_NOT_READY;
  out->primary = grip_demo_value(s_demo_index++);
  out->secondary = HS_VALUE_INVALID;
  return HS_OK;
}

#else

#include <stdio.h>

/*
 * 标定常数：每 0.1kg 对应的净计数（raw - 皮重）。
 * 2026-09-03 对装入握力计机构的 70kg 传感器现场标定：厨房秤作为参考，
 * 1.286kg -> 4905 counts；2.591kg -> 10261 counts（两次均值），卸载残余
 * 296 counts。轻载点给出 381.4 counts/0.1kg，高载点经零点残余校正给出
 * 384.6 counts/0.1kg，取整为 384。烧录后复测 2.589kg 显示 2.6kg；但该
 * 机构一次加载-卸载后零点会回弹数百 counts，轻载精度尚未验收。该结果只验证
 * 到 2.591kg，70kg 全量程仍需经机械预加载、重新去皮和标准力源复核。
 */
#ifndef GRIP_COUNTS_PER_01KG
#define GRIP_COUNTS_PER_01KG 384
#endif

#define GRIP_WINDOW_MS 3000 /* 峰值保持窗口：给用户约 3s 用力 */
#define GRIP_MIN_NET_COUNTS                           \
  2000 /* 低于此净计数视为「没在用力」→ HS_UNSTABLE \
        */
#define GRIP_READ_TIMEOUT_MS 200 /* 单次读就绪超时 */
#define GRIP_TARE_TIMES 8        /* 去皮平均次数 */

static hx711_t s_grip;
static uint8_t s_ready; /* 只初始化+去皮一次；app 每轮都会调 init，此处幂等 */

hs_status_t hx711_grip_init(void) {
  if (s_ready) return HS_OK; /* 已就绪：跳过，保留已标定的皮重 */

  s_grip.sck_port = HX711_G_SCK_PORT;
  s_grip.sck_pin = HX711_G_SCK_PIN;
  s_grip.dout_port = HX711_G_DOUT_PORT;
  s_grip.dout_pin = HX711_G_DOUT_PIN;
  s_grip.gain = HX711_GAIN_A128; /* 称重用通道 A/128 最灵敏 */
  hx711_init(&s_grip);           /* hx711_init 会把 offset 清零 */

#if defined(YUHAO_BRINGUP)
  printf("[hx711_grip] DOUT ready before tare=%d\r\n",
         hx711_is_ready(&s_grip));
#endif

  /* 空载去皮：首次初始化时假定未握把手，多次平均作皮重基线。
     失败则保持 s_ready=0，下一轮 init 再试（避免把接线故障固化）。 */
  if (hx711_tare(&s_grip, GRIP_TARE_TIMES, 500) != 0) {
#if defined(YUHAO_BRINGUP)
    printf("[hx711_grip] tare failed: DOUT stayed not-ready\r\n");
#endif
    return HS_TIMEOUT; /* DOUT 一直不就绪 → 接线/供电问题 */
  }
#if defined(YUHAO_BRINGUP)
  printf("[hx711_grip] tare offset=%ld\r\n", (long)s_grip.offset);
#endif
  s_ready = 1;
  return HS_OK;
}

hs_status_t hx711_grip_measure(hs_sample_t *out) {
  if (out == 0) return HS_NOT_READY;
  out->primary = HS_VALUE_INVALID;
  out->secondary = HS_VALUE_INVALID;

  int32_t peak_net = 0; /* 窗口内净计数峰值（约定：用力使 raw 增大） */
  int32_t min_net = 0;
  int32_t max_net = 0;
  int any = 0;
  uint32_t t0 = HAL_GetTick();

  while (HAL_GetTick() - t0 < GRIP_WINDOW_MS) {
    int32_t raw;
    if (hx711_read_raw(&s_grip, &raw, GRIP_READ_TIMEOUT_MS) != 0) {
      return HS_TIMEOUT; /* 读不到就绪，判为超时 */
    }
    any = 1;
    int32_t net = raw - s_grip.offset;
    if (net < min_net) min_net = net;
    if (net > max_net) max_net = net;
    if (net > peak_net) peak_net = net;
  }
  if (!any) return HS_TIMEOUT;
#if defined(YUHAO_BRINGUP)
  printf("[hx711_grip] window net_min=%ld net_max=%ld\r\n", (long)min_net,
         (long)max_net);
#endif
  if (peak_net < GRIP_MIN_NET_COUNTS) return HS_UNSTABLE; /* 没测到有效握力 */

  /* 净计数峰值 → kg×10，四舍五入 */
  int32_t grip_kg_x10 =
      (int32_t)(((int64_t)peak_net + GRIP_COUNTS_PER_01KG / 2) /
                GRIP_COUNTS_PER_01KG);
  out->primary = grip_kg_x10;
  return HS_OK;
}

#endif /* GRIP_DEMO_MODE */

#else /* 本板 MODULE_SET 未包含该模块：桩替代，仅供 app 注册表链接 */

hs_status_t hx711_grip_init(void) { return HS_NOT_IMPLEMENTED; }
hs_status_t hx711_grip_measure(hs_sample_t *out) {
  (void)out;
  return HS_NOT_IMPLEMENTED;
}

#endif
