/**
 * @file    storage.c
 * @owner   刘晏铭
 * TODO(刘晏铭)：在 EEPROM 区实现记录顺序写+磨损页切换+读回校验。
 * 可直接调用 bsp/flash_eeprom.h 的 eeprom_read/erase/write 与 record_codec 的
 * CRC。
 */
#include "storage.h"

#if defined(MODULE_ENABLED_STORAGE)
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
