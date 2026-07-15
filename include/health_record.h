/**
 * @file    health_record.h
 * @brief   健康体测数据格式定稿（片内 Flash 存储与 BLE/UART 传输共用）。
 * @owner   王宇浩（组长）
 *
 * 本文件是全队第一周必须冻结的契约之一。任何改动都必须：
 *   1) 同步 RECORD_VERSION / FRAME_VERSION；
 *   2) 同步更新 docs/接口文档.md 与 web/dashboard.html 的解析逻辑；
 *   3) 三人评审通过后方可合入 dev。
 *
 * 字节序：STM32F103 为小端（little-endian）。record 与 frame 的所有多字节
 * 整数字段一律以小端在 Flash / BLE 上传输。web 端解析用 DataView +
 * littleEndian=true。
 *
 * 所有物理量采用定点整数（避免 -Os 下引入软浮点、也便于 CRC 与跨平台一致）。
 * 每个字段都注明单位与缩放倍率，UI 层负责还原为带小数的显示值。
 */
#ifndef HEALTH_RECORD_H
#define HEALTH_RECORD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 记录结构版本号：字段布局一旦变化必须 +1 */
#define RECORD_VERSION 0x01u

/* 无效/未测量占位值：任一指标未采到时填此值，UI 显示为 "--" */
#define HS_VALUE_INVALID ((int32_t)0x7FFFFFFF)

/**
 * @brief 一条完整体测记录。共 8 项指标 + 综合评分。
 *
 * 结构体按 4 字节字段自然对齐，无隐藏填充；sizeof 固定，跨编译器一致。
 * CRC16 覆盖范围：从 version 起到 score 止（即除 crc16 自身外的全部字节）。
 */
typedef struct {
  uint16_t version;   /* = RECORD_VERSION */
  uint16_t reserved;  /* 对齐/预留，写入 0 */
  uint32_t timestamp; /* 上电以来的秒数或 RTC 秒；本项目用 SysTick 秒计数 */

  /* ---- 8 项测量指标（顺序即 hs_item_t 顺序）---- */
  int32_t height_mm;      /* 身高，单位 mm，例 1732 = 173.2cm */
  int32_t weight_g;       /* 体重，单位 g，例 65300 = 65.3kg */
  int32_t bmi_x100;       /* BMI，×100，例 2180 = 21.80 */
  int32_t bodyfat_x10;    /* 体脂率，×10 百分数，例 185 = 18.5% */
  int32_t heart_rate_bpm; /* 心率，单位 bpm */
  int32_t spo2_x10;       /* 血氧饱和度，×10 百分数，例 985 = 98.5% */
  int32_t balance_x10;    /* 平衡晃动指数，×10（越小越稳），自定义量纲 */
  int32_t grip_kg_x10;    /* 握力，×10 kg，例 352 = 35.2kg */
  int32_t reaction_ms;    /* 反应时间，单位 ms */

  int32_t score; /* 综合健康评分，0~100 */

  uint16_t crc16; /* CRC16-CCITT(0x1021, init 0xFFFF)，见 algorithms/crc16.h */
  uint16_t pad;   /* 对齐到 4 字节边界，写入 0 */
} measurement_record_t;

/* 编译期断言：确保结构体大小锁定为 52 字节，防止编译器擅自填充导致跨端不一致 */
typedef char _assert_record_size[(sizeof(measurement_record_t) == 52) ? 1 : -1];

/* CRC 覆盖的字节数（结构体总长减去末尾 crc16(2) + pad(2)） */
#define RECORD_CRC_LEN (sizeof(measurement_record_t) - 4u)

/* -------------------------------------------------------------------------
 *  UART / BLE 数据帧格式
 *
 *  字节布局（全部小端）：
 *    +------+------+------+--------+---------------------+--------+
 *    | SOF0 | SOF1 | TYPE | LEN(2) | PAYLOAD(LEN 字节)   | CRC(2) |
 *    +------+------+------+--------+---------------------+--------+
 *      0xAA   0x55   1B     2B LE    LEN 字节              2B LE
 *
 *  - SOF：固定 0xAA 0x55 帧头，用于接收端重同步；
 *  - TYPE：帧类型，见 frame_type_t；
 *  - LEN：PAYLOAD 字节数（小端 uint16）；
 *  - PAYLOAD：按 TYPE 解释，FRAME_TYPE_RECORD 时即 measurement_record_t
 * 原始字节；
 *  - CRC：CRC16-CCITT 覆盖 [TYPE, LEN(2), PAYLOAD]（不含 SOF），小端。
 *
 *  web/dashboard.html 的 parseFrame() 必须与此严格一致。
 * ------------------------------------------------------------------------- */
#define FRAME_SOF0 0xAAu
#define FRAME_SOF1 0x55u
#define FRAME_HEADER_LEN 5u /* SOF0+SOF1+TYPE+LEN(2) */
#define FRAME_CRC_LEN 2u
#define FRAME_OVERHEAD (FRAME_HEADER_LEN + FRAME_CRC_LEN)

typedef enum {
  FRAME_TYPE_RECORD = 0x01, /* payload = measurement_record_t */
  FRAME_TYPE_ACK = 0x02,    /* payload = 0 字节 */
  FRAME_TYPE_HELLO = 0x10,  /* payload = ASCII 版本串，握手/自检用 */
} frame_type_t;

#ifdef __cplusplus
}
#endif

#endif /* HEALTH_RECORD_H */
