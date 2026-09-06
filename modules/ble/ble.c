/**
 * @file    ble.c
 * @owner   刘晏铭
 *
 * BLE 只负责把已经收尾的 measurement_record_t 编码为冻结数据帧并透传。
 * 模块配置由 HM-10/JDY-23 的出厂设置负责：USART2 为 9600 8N1，模块上电后
 * 直接进入透传模式。网页命令由 ESP32 从 GPIO17 转发到 PA3，采用换行结尾的
 * 简短 ASCII 命令；记录响应仍使用带 CRC 的冻结二进制帧。
 */
#include "ble.h"

#include <string.h>

#include "record_codec.h"

#if defined(MODULE_ENABLED_BLE)

#include "uart.h" /* ble_uart_send，仅固件构建可见 */

static int s_ble_ready;
static char s_command[24];
static uint8_t s_command_len;
static ble_field_t s_pending_field;
static int32_t s_pending_value;

static ble_field_t parse_field(const char *text, size_t len) {
  if (len == 1u) {
    switch (text[0]) {
      case 'H': return BLE_FIELD_HEIGHT;
      case 'W': return BLE_FIELD_WEIGHT;
      case 'F': return BLE_FIELD_BODYFAT;
      case 'O': return BLE_FIELD_SPO2;
      case 'B': return BLE_FIELD_BALANCE;
      case 'G': return BLE_FIELD_GRIP;
      case 'R': return BLE_FIELD_REACTION;
      default: return BLE_FIELD_NONE;
    }
  }
  if (len == 2u && text[0] == 'H' && text[1] == 'R') {
    return BLE_FIELD_HEART_RATE;
  }
  return BLE_FIELD_NONE;
}

static int parse_int32(const char *text, int32_t *out) {
  int negative = 0;
  int64_t value = 0;
  if (*text == '-') {
    negative = 1;
    ++text;
  }
  if (*text == '\0') return 0;
  while (*text != '\0') {
    if (*text < '0' || *text > '9') return 0;
    value = value * 10 + (*text - '0');
    if ((!negative && value > 2147483647LL) ||
        (negative && value > 2147483648LL)) {
      return 0;
    }
    ++text;
  }
  *out = negative ? (int32_t)-value : (int32_t)value;
  return 1;
}

static int parse_set_command(void) {
  const char *field_text = &s_command[4];
  const char *separator = strchr(field_text, ' ');
  if (separator == NULL || separator == field_text) return 0;
  const ble_field_t field =
      parse_field(field_text, (size_t)(separator - field_text));
  int32_t value;
  if (field == BLE_FIELD_NONE || !parse_int32(separator + 1, &value)) return 0;
  s_pending_field = field;
  s_pending_value = value;
  return 1;
}

static ble_command_t parse_command(void) {
  s_command[s_command_len] = '\0';
  ble_command_t result = BLE_CMD_UNKNOWN;
  if (strcmp(s_command, "PING") == 0) {
    result = BLE_CMD_PING;
  } else if (strcmp(s_command, "GET CURRENT") == 0) {
    result = BLE_CMD_GET_CURRENT;
  } else if (strcmp(s_command, "GET HISTORY") == 0) {
    result = BLE_CMD_GET_HISTORY;
  } else if (strncmp(s_command, "SET ", 4u) == 0 && parse_set_command()) {
    result = BLE_CMD_SET_FIELD;
  } else if (strcmp(s_command, "APPLY") == 0) {
    result = BLE_CMD_APPLY;
  } else if (strcmp(s_command, "SAVE") == 0) {
    result = BLE_CMD_SAVE;
  }
  s_command_len = 0;
  return result;
}

hs_status_t ble_init(void) {
  /* app 已先调用 uart_init()；BLE 模块须已上电并处于透传模式。 */
  s_ble_ready = 1;
  s_command_len = 0;
  s_pending_field = BLE_FIELD_NONE;
  s_pending_value = 0;
  return HS_OK;
}

hs_status_t ble_send_record(const measurement_record_t *rec) {
  if (!s_ble_ready || rec == NULL) return HS_NOT_READY;

  /* BLE 自己收尾副本，避免调用方漏算记录 CRC 或被发送过程修改。 */
  measurement_record_t finalized = *rec;
  record_finalize(&finalized);

  uint8_t frame[FRAME_OVERHEAD + sizeof(measurement_record_t)];
  size_t n = frame_encode_record(&finalized, frame, sizeof(frame));
  if (n == 0) return HS_NOT_READY;
  ble_uart_send(frame, n);
  return HS_OK;
}

hs_status_t ble_send_status(const char *text) {
  if (!s_ble_ready || text == NULL) return HS_NOT_READY;
  const size_t text_len = strlen(text);
  if (text_len > 63u) return HS_NOT_READY;
  uint8_t frame[FRAME_OVERHEAD + 63u];
  const size_t n = frame_encode((uint8_t)FRAME_TYPE_HELLO,
                                (const uint8_t *)text, (uint16_t)text_len,
                                frame, sizeof(frame));
  if (n == 0) return HS_NOT_READY;
  ble_uart_send(frame, n);
  return HS_OK;
}

ble_command_t ble_poll_command(void) {
  uint8_t byte;
  while (ble_uart_receive_byte(&byte)) {
    if (byte == '\r') continue;
    if (byte == '\n') {
      if (s_command_len == 0) continue;
      return parse_command();
    }
    if (byte >= 'a' && byte <= 'z') byte = (uint8_t)(byte - 'a' + 'A');
    if (byte < 0x20 || byte > 0x7e || s_command_len >= sizeof(s_command) - 1u) {
      s_command_len = 0;
      return BLE_CMD_UNKNOWN;
    }
    s_command[s_command_len++] = (char)byte;
  }
  return BLE_CMD_NONE;
}

int ble_get_pending_set(ble_field_t *field, int32_t *value) {
  if (field == NULL || value == NULL || s_pending_field == BLE_FIELD_NONE) {
    return 0;
  }
  *field = s_pending_field;
  *value = s_pending_value;
  s_pending_field = BLE_FIELD_NONE;
  return 1;
}

#else /* 未启用：桩替代 */

hs_status_t ble_init(void) { return HS_NOT_IMPLEMENTED; }
hs_status_t ble_send_record(const measurement_record_t *rec) {
  (void)rec;
  return HS_NOT_IMPLEMENTED;
}
hs_status_t ble_send_status(const char *text) {
  (void)text;
  return HS_NOT_IMPLEMENTED;
}
ble_command_t ble_poll_command(void) { return BLE_CMD_NONE; }
int ble_get_pending_set(ble_field_t *field, int32_t *value) {
  (void)field;
  (void)value;
  return 0;
}

#endif
