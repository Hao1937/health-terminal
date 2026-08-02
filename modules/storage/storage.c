/**
 * @file    storage.c
 * @owner   刘晏铭
 *
 * 片内 Flash 模拟 EEPROM 上的记录日志：顺序追加、写满整块 4 页后一次性擦除
 * 重来（不是逐条淘汰最旧记录的复杂环形）。这样重启后只需扫描"从槽 0 开始
 * 连续有效"的前缀即可恢复写入位置，不需要额外记录跨断电的写入序号——对课程
 * 项目的历史记录场景是足够的简化，不追求工业级磨损均衡。
 *
 * 已知局限（有意不做）：写入中途断电可能留下半写坏槽，下次 storage_init()
 * 会把它当成边界直接丢弃其后的写入位置，不会尝试修复。
 */
#include "storage.h"

#if defined(MODULE_ENABLED_STORAGE)

#include "flash_eeprom.h"
#include "record_codec.h"

/* 每页固定放置 19 条记录，页尾保留 36 字节，4 页共 76 条。 */
#define RECS_PER_PAGE (EEPROM_PAGE_SIZE / sizeof(measurement_record_t))
#define TOTAL_SLOTS (RECS_PER_PAGE * EEPROM_PAGE_COUNT)

static uint16_t s_count;     /* 当前有效记录数 */
static uint16_t s_write_idx; /* 下一条写入的物理槽位，顺序写不留洞 */

static uint32_t slot_offset(uint16_t slot) {
  uint16_t page = (uint16_t)(slot / RECS_PER_PAGE);
  uint16_t slot_in_page = (uint16_t)(slot % RECS_PER_PAGE);
  return (uint32_t)page * EEPROM_PAGE_SIZE +
         (uint32_t)slot_in_page * sizeof(measurement_record_t);
}

hs_status_t storage_init(void) {
  s_count = 0;
  for (uint16_t i = 0; i < TOTAL_SLOTS; ++i) {
    measurement_record_t rec;
    if (eeprom_read(slot_offset(i), &rec, sizeof(rec)) != 0) break;
    if (!record_verify(&rec)) break; /* 第一个无效(擦除态)槽即为写入边界 */
    ++s_count;
  }
  s_write_idx = s_count;
  return HS_OK;
}

hs_status_t storage_append(const measurement_record_t *rec) {
  if (s_write_idx >= TOTAL_SLOTS) {
    /* 存满：一次性擦除 4 页重新开始，简单版"磨损轮转" */
    for (uint32_t p = 0; p < EEPROM_PAGE_COUNT; ++p) {
      if (eeprom_erase_page(p) != 0) return HS_TIMEOUT;
    }
    s_write_idx = 0;
    s_count = 0;
  } else if (s_write_idx % RECS_PER_PAGE == 0) {
    uint32_t page = s_write_idx / RECS_PER_PAGE;
    if (eeprom_erase_page(page) != 0) return HS_TIMEOUT;
  }

  measurement_record_t tmp = *rec;
  record_finalize(&tmp); /* 接口文档已注明内部补 CRC，幂等 */
  if (eeprom_write(slot_offset(s_write_idx), &tmp, sizeof(tmp)) != 0) {
    return HS_TIMEOUT;
  }
  ++s_write_idx;
  if (s_count < TOTAL_SLOTS) ++s_count;
  return HS_OK;
}

uint16_t storage_count(void) { return s_count; }

hs_status_t storage_read(uint16_t idx, measurement_record_t *out) {
  if (idx >= s_count) return HS_NOT_READY;
  if (eeprom_read(slot_offset(idx), out, sizeof(*out)) != 0) {
    return HS_NOT_READY;
  }
  if (!record_verify(out)) return HS_NOT_READY;
  return HS_OK;
}

#else
hs_status_t storage_init(void) { return HS_NOT_IMPLEMENTED; }
hs_status_t storage_append(const measurement_record_t *rec) {
  (void)rec;
  return HS_NOT_IMPLEMENTED;
}
uint16_t storage_count(void) { return 0; }
hs_status_t storage_read(uint16_t idx, measurement_record_t *out) {
  (void)idx;
  (void)out;
  return HS_NOT_IMPLEMENTED;
}
#endif
