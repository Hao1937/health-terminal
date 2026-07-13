/*
 * STM32F103C8Tx 链接脚本（GCC）
 *   Flash 64KB @ 0x08000000，RAM 20KB @ 0x20000000，页大小 1KB。
 *   为「片内 Flash 模拟 EEPROM」预留最后 4 页（4KB）：0x0800F000~0x0800FFFF。
 *   历史记录存储区与代码区物理隔离，storage 模块只在 EEPROM 区读写。
 * 负责人：王宇浩（组长）；EEPROM 区使用者：刘晏铭（storage 模块）
 */
ENTRY(Reset_Handler)

_estack = ORIGIN(RAM) + LENGTH(RAM);   /* 栈顶（RAM 末端） */
_Min_Heap_Size  = 0x200;               /* 512B 堆 */
_Min_Stack_Size = 0x400;               /* 1KB 栈 */

MEMORY
{
  RAM    (xrw) : ORIGIN = 0x20000000, LENGTH = 20K
  FLASH  (rx)  : ORIGIN = 0x08000000, LENGTH = 60K   /* 代码区：64K - 4K(EEPROM) */
  EEPROM (r)   : ORIGIN = 0x0800F000, LENGTH = 4K    /* 模拟 EEPROM，4 页 */
}

/* 供 storage 模块引用的 EEPROM 区边界符号 */
_eeprom_start = ORIGIN(EEPROM);
_eeprom_size  = LENGTH(EEPROM);
_eeprom_page_size = 1K;

SECTIONS
{
  .isr_vector : { . = ALIGN(4); KEEP(*(.isr_vector)) . = ALIGN(4); } >FLASH

  .text :
  {
    . = ALIGN(4);
    *(.text) *(.text*)
    *(.glue_7) *(.glue_7t)
    *(.eh_frame)
    KEEP (*(.init)) KEEP (*(.fini))
    . = ALIGN(4);
    _etext = .;
  } >FLASH

  .rodata : { . = ALIGN(4); *(.rodata) *(.rodata*) . = ALIGN(4); } >FLASH
  .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
  .ARM : { __exidx_start = .; *(.ARM.exidx*) __exidx_end = .; } >FLASH

  .preinit_array : { PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*)) PROVIDE_HIDDEN (__preinit_array_end = .); } >FLASH
  .init_array : { PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*))) KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .); } >FLASH
  .fini_array : { PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*))) KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .); } >FLASH

  _sidata = LOADADDR(.data);
  .data :
  {
    . = ALIGN(4); _sdata = .;
    *(.data) *(.data*)
    . = ALIGN(4); _edata = .;
  } >RAM AT> FLASH

  .bss :
  {
    . = ALIGN(4); _sbss = .; __bss_start__ = _sbss;
    *(.bss) *(.bss*) *(COMMON)
    . = ALIGN(4); _ebss = .; __bss_end__ = _ebss;
  } >RAM

  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . ); PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM

  /DISCARD/ : { libc.a ( * ) libm.a ( * ) libgcc.a ( * ) }
  .ARM.attributes 0 : { *(.ARM.attributes) }
}
