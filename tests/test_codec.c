/**
 * @file    test_codec.c
 * @brief   CRC16、record 校验、UART/BLE 帧编解码往返一致性测试。
 * @owner   王宇浩（组长）
 */
#include "test_util.h"
#include "crc16.h"
#include "record_codec.h"
#include <string.h>

/* CRC16-CCITT 已知向量："123456789" → 0x29B1 */
static void test_crc_known_vector(void)
{
    printf("test_crc_known_vector\n");
    const char *s = "123456789";
    uint16_t c = crc16_ccitt((const uint8_t *)s, 9);
    CHECK_EQ(c, 0x29B1);
}

static measurement_record_t make_sample(void)
{
    measurement_record_t r;
    memset(&r, 0, sizeof(r));
    r.timestamp = 12345;
    r.height_mm = 1732;
    r.weight_g = 65300;
    r.bmi_x100 = 2180;
    r.bodyfat_x10 = 185;
    r.heart_rate_bpm = 72;
    r.spo2_x10 = 985;
    r.balance_x10 = 25;
    r.grip_kg_x10 = 352;
    r.reaction_ms = 240;
    r.score = 88;
    return r;
}

/* record 收尾后自校验通过，篡改任一字节后校验失败 */
static void test_record_crc(void)
{
    printf("test_record_crc\n");
    measurement_record_t r = make_sample();
    record_finalize(&r);
    CHECK_EQ(record_verify(&r), 1);

    measurement_record_t bad = r;
    bad.heart_rate_bpm ^= 0x01; /* 篡改 1 bit */
    CHECK_EQ(record_verify(&bad), 0);
}

/* 帧编码→解码往返，字段完全还原 */
static void test_frame_roundtrip(void)
{
    printf("test_frame_roundtrip\n");
    measurement_record_t r = make_sample();
    record_finalize(&r);

    uint8_t buf[128];
    size_t n = frame_encode_record(&r, buf, sizeof(buf));
    CHECK_EQ(n, FRAME_OVERHEAD + sizeof(measurement_record_t));
    CHECK_EQ(buf[0], FRAME_SOF0);
    CHECK_EQ(buf[1], FRAME_SOF1);
    CHECK_EQ(buf[2], FRAME_TYPE_RECORD);

    measurement_record_t out;
    memset(&out, 0, sizeof(out));
    size_t used = frame_decode_record(buf, n, &out);
    CHECK_EQ(used, n);
    CHECK_EQ(out.height_mm, 1732);
    CHECK_EQ(out.weight_g, 65300);
    CHECK_EQ(out.grip_kg_x10, 352);
    CHECK_EQ(out.score, 88);
    CHECK_EQ(memcmp(&r, &out, sizeof(r)), 0);
}

/* 帧 CRC 篡改必须被拒 */
static void test_frame_tamper(void)
{
    printf("test_frame_tamper\n");
    measurement_record_t r = make_sample();
    record_finalize(&r);
    uint8_t buf[128];
    size_t n = frame_encode_record(&r, buf, sizeof(buf));

    buf[10] ^= 0xFF; /* 破坏 payload 某字节 */
    measurement_record_t out;
    CHECK_EQ(frame_decode_record(buf, n, &out), 0);

    /* 容量不足时编码应返回 0 */
    uint8_t tiny[4];
    CHECK_EQ(frame_encode_record(&r, tiny, sizeof(tiny)), 0);
}

int main(void)
{
    printf("== test_codec ==\n");
    test_crc_known_vector();
    test_record_crc();
    test_frame_roundtrip();
    test_frame_tamper();
    return TEST_SUMMARY();
}
