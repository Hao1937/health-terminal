/**
 * @file    syscalls.c
 * @brief   printf 重定向到调试串口（USART1），及 newlib 最小系统调用桩。
 * @owner   王宇浩（组长）
 *
 * 配合 --specs=nosys.specs 使用。bsp/uart 提供 g_debug_uart 句柄与阻塞发送。
 */
#include <errno.h>
#include <sys/stat.h>

#include "stm32f1xx_hal.h"

/* 在 bsp/uart.c 中定义并初始化 */
extern UART_HandleTypeDef g_debug_uart;

/* newlib 通过 _write 输出；这里把 fd=1/2 都送到调试串口 */
int _write(int fd, const char *ptr, int len) {
  (void)fd;
  HAL_UART_Transmit(&g_debug_uart, (uint8_t *)ptr, (uint16_t)len,
                    HAL_MAX_DELAY);
  return len;
}

/* 以下为 nosys 之外仍可能被引用的最小桩，保证链接通过 */
int _read(int fd, char *ptr, int len) {
  (void)fd;
  (void)ptr;
  (void)len;
  return 0;
}
int _close(int fd) {
  (void)fd;
  return -1;
}
int _fstat(int fd, struct stat *st) {
  (void)fd;
  st->st_mode = S_IFCHR;
  return 0;
}
int _isatty(int fd) {
  (void)fd;
  return 1;
}
int _lseek(int fd, int off, int dir) {
  (void)fd;
  (void)off;
  (void)dir;
  return 0;
}
void _exit(int code) {
  (void)code;
  while (1) {
  }
}
int _getpid(void) { return 1; }
int _kill(int pid, int sig) {
  (void)pid;
  (void)sig;
  errno = EINVAL;
  return -1;
}
