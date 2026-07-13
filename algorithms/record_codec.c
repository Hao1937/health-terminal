/**
 * @file    record_codec.c
 * @owner   王宇浩（组长）
 */
#include "record_codec.h"
#include "crc16.h"

void record_finalize(measurement_record_t *rec)
{
    rec->version = RECORD_VERSION;
    rec->reserved = 0;
    rec->pad = 0;
    rec->crc16 = crc16_ccitt((const uint8_t *)rec, RECORD_CRC_LEN);
}

int record_verify(const measurement_record_t *rec)
{
    if (rec->version != RECORD_VERSION) return 0;
    uint16_t c = crc16_ccitt((const uint8_t *)rec, RECORD_CRC_LEN);
    return (c == rec->crc16) ? 1 : 0;
}

/* 小端写入辅助 */
static void put_u16le(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)(v >> 8);
}
static uint16_t get_u16le(const uint8_t *p)
{
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

size_t frame_encode_record(const measurement_record_t *rec, uint8_t *out, size_t cap)
{
    const uint16_t payload_len = (uint16_t)sizeof(measurement_record_t);
    const size_t total = FRAME_OVERHEAD + payload_len;
    if (cap < total) return 0;

    out[0] = FRAME_SOF0;
    out[1] = FRAME_SOF1;
    out[2] = (uint8_t)FRAME_TYPE_RECORD;
    put_u16le(&out[3], payload_len);

    /* record 各字段本就是小端存储（LE 主机/STM32），直接按字节拷贝 */
    const uint8_t *src = (const uint8_t *)rec;
    for (uint16_t i = 0; i < payload_len; ++i) out[FRAME_HEADER_LEN + i] = src[i];

    /* CRC 覆盖 [TYPE .. PAYLOAD]，即从 out[2] 起、长度 1+2+payload_len */
    uint16_t crc = crc16_ccitt(&out[2], (size_t)(3 + payload_len));
    put_u16le(&out[FRAME_HEADER_LEN + payload_len], crc);
    return total;
}

size_t frame_decode_record(const uint8_t *in, size_t len, measurement_record_t *rec)
{
    if (len < FRAME_OVERHEAD) return 0;
    if (in[0] != FRAME_SOF0 || in[1] != FRAME_SOF1) return 0;
    if (in[2] != (uint8_t)FRAME_TYPE_RECORD) return 0;

    uint16_t payload_len = get_u16le(&in[3]);
    if (payload_len != (uint16_t)sizeof(measurement_record_t)) return 0;

    size_t total = FRAME_OVERHEAD + payload_len;
    if (len < total) return 0;

    uint16_t crc_calc = crc16_ccitt(&in[2], (size_t)(3 + payload_len));
    uint16_t crc_rx = get_u16le(&in[FRAME_HEADER_LEN + payload_len]);
    if (crc_calc != crc_rx) return 0;

    uint8_t *dst = (uint8_t *)rec;
    for (uint16_t i = 0; i < payload_len; ++i) dst[i] = in[FRAME_HEADER_LEN + i];

    /* 载荷自身也带 record CRC，进一步校验 */
    if (!record_verify(rec)) return 0;
    return total;
}
