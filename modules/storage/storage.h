/**
 * @file    storage.h
 * @brief   片内 Flash 历史记录存储（基于 bsp/flash_eeprom 的记录环形区）。
 * @owner   刘晏铭
 */
#ifndef MODULE_STORAGE_H
#define MODULE_STORAGE_H

#include "health_if.h"
#include "health_record.h"

hs_status_t storage_init(void);
/** @brief 追加一条记录（内部 record_finalize 补 CRC）。 */
hs_status_t storage_append(const measurement_record_t *rec);
/** @brief 已存记录条数。 */
uint16_t storage_count(void);
/** @brief 读取第 idx 条（0 最旧）；CRC 校验失败返回 HS_NOT_READY。 */
hs_status_t storage_read(uint16_t idx, measurement_record_t *out);

#endif /* MODULE_STORAGE_H */
