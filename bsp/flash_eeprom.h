/**
 * @file    flash_eeprom.h
 * @brief   片内 Flash 模拟 EEPROM：读写链接脚本预留的最后 4 页。
 * @owner   王宇浩（组长）；主要使用者：刘晏铭（storage 历史记录模块）
 *
 * EEPROM 区由 STM32F103C8Tx_FLASH.ld 隔离出（0x0800F000，4KB=4 页，页 1KB）。
 * 提供「按字节偏移读」「按页擦写」的裸接口，storage 模块在其上实现记录环形存储。
 */
#ifndef BSP_FLASH_EEPROM_H
#define BSP_FLASH_EEPROM_H

#include <stdint.h>
#include <stddef.h>

#define EEPROM_PAGE_SIZE  1024u
#define EEPROM_PAGE_COUNT 4u
#define EEPROM_TOTAL_SIZE (EEPROM_PAGE_SIZE * EEPROM_PAGE_COUNT)

/** @brief 返回 EEPROM 区起始物理地址（来自链接脚本 _eeprom_start）。 */
uint32_t eeprom_base_addr(void);

/**
 * @brief 从 EEPROM 区偏移 offset 读 len 字节到 buf。
 * @return 0 成功；-1 越界。
 */
int eeprom_read(uint32_t offset, void *buf, size_t len);

/**
 * @brief 擦除 EEPROM 区第 page 页（0..EEPROM_PAGE_COUNT-1）。
 * @return 0 成功；-1 参数错；-2 HAL 失败。
 */
int eeprom_erase_page(uint32_t page);

/**
 * @brief 向 EEPROM 区偏移 offset 写 len 字节（须半字对齐，调用方保证目标已擦除）。
 * @return 0 成功；-1 越界/未对齐；-2 HAL 失败。
 */
int eeprom_write(uint32_t offset, const void *buf, size_t len);

#endif /* BSP_FLASH_EEPROM_H */
