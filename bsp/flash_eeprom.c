/**
 * @file    flash_eeprom.c
 * @owner   王宇浩（组长）
 */
#include "flash_eeprom.h"
#include "stm32f1xx_hal.h"

/* 链接脚本导出的 EEPROM 区符号 */
extern uint32_t _eeprom_start;

uint32_t eeprom_base_addr(void)
{
    return (uint32_t)&_eeprom_start;
}

int eeprom_read(uint32_t offset, void *buf, size_t len)
{
    if (offset + len > EEPROM_TOTAL_SIZE) return -1;
    const uint8_t *src = (const uint8_t *)(eeprom_base_addr() + offset);
    uint8_t *dst = (uint8_t *)buf;
    for (size_t i = 0; i < len; ++i) dst[i] = src[i];
    return 0;
}

int eeprom_erase_page(uint32_t page)
{
    if (page >= EEPROM_PAGE_COUNT) return -1;
    FLASH_EraseInitTypeDef er = {0};
    uint32_t err = 0;
    er.TypeErase = FLASH_TYPEERASE_PAGES;
    er.PageAddress = eeprom_base_addr() + page * EEPROM_PAGE_SIZE;
    er.NbPages = 1;

    HAL_FLASH_Unlock();
    HAL_StatusTypeDef st = HAL_FLASHEx_Erase(&er, &err);
    HAL_FLASH_Lock();
    return (st == HAL_OK) ? 0 : -2;
}

int eeprom_write(uint32_t offset, const void *buf, size_t len)
{
    if (offset + len > EEPROM_TOTAL_SIZE) return -1;
    if (offset & 0x1u) return -1;           /* 半字对齐 */

    const uint8_t *src = (const uint8_t *)buf;
    uint32_t addr = eeprom_base_addr() + offset;
    int rc = 0;

    HAL_FLASH_Unlock();
    for (size_t i = 0; i < len; i += 2) {
        uint16_t half = src[i];
        if (i + 1 < len) half |= (uint16_t)src[i + 1] << 8;
        else half |= 0xFF00u;               /* 奇数尾字节高位补 0xFF */
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr, half) != HAL_OK) {
            rc = -2;
            break;
        }
        addr += 2;
    }
    HAL_FLASH_Lock();
    return rc;
}
