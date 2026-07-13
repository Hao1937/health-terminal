/**
 * @file    health_if.h
 * @brief   全体测量项统一传感器接口与状态码（第一周冻结契约）。
 * @owner   王宇浩（组长）
 *
 * 设计原则（不过度设计）：
 *   - 所有模块共用同一套状态码 hs_status_t；
 *   - 每个测量项实现统一签名的 init / measure 两个函数；
 *   - measure 结果写入通用 hs_sample_t（primary + 可选 secondary，覆盖
 *     心率/血氧这类双输出传感器）；
 *   - app 层用一张 hs_sensor_t 注册表把「测量项」映射到「函数指针」，
 *     状态机按项遍历采集，不需要为每个传感器写分支。
 *
 * 每个模块的桩实现一律返回 HS_NOT_IMPLEMENTED，各自把模块实现进度推进到 HS_OK。
 */
#ifndef HEALTH_IF_H
#define HEALTH_IF_H

#include <stdint.h>
#include "health_record.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 统一状态码。所有 init/measure 函数都返回它。
 */
typedef enum {
    HS_OK = 0,            /* 成功，数据有效 */
    HS_NOT_READY,        /* 传感器/数据尚未就绪，调用方应稍后重试 */
    HS_UNSTABLE,         /* 读到数据但未达稳定判据（如体重未稳、姿态抖动） */
    HS_TIMEOUT,          /* 等待超时（如 HC-SR04 无回波、HX711 长期不 ready） */
    HS_NOT_IMPLEMENTED,  /* 模块桩：功能尚未实现，仅占位 */
} hs_status_t;

/**
 * @brief 8 项测量项枚举，顺序与 measurement_record_t 中指标字段顺序一致。
 */
typedef enum {
    HS_ITEM_HEIGHT = 0,  /* 身高          —— 查樊听 */
    HS_ITEM_WEIGHT,      /* 体重          —— 查樊听 */
    HS_ITEM_BODY,        /* BMI/体脂率    —— 算法(王宇浩) 由身高体重导出 */
    HS_ITEM_HR_SPO2,     /* 心率+血氧     —— 王宇浩 */
    HS_ITEM_BALANCE,     /* 平衡晃动指数  —— 刘晏铭 */
    HS_ITEM_GRIP,        /* 握力          —— 王宇浩 */
    HS_ITEM_REACTION,    /* 反应时间      —— 刘晏铭 */
    HS_ITEM_COUNT,       /* == 有效测量项数量，仅计数用 */
} hs_item_t;

/**
 * @brief 通用采样结果。
 *
 * 大多数传感器只用 primary；双输出传感器（MAX30102 心率+血氧）用 secondary。
 * unit_hint 仅供调试打印，不参与逻辑。
 */
typedef struct {
    int32_t primary;    /* 主指标定点值，单位见对应 record 字段注释 */
    int32_t secondary;  /* 次指标（如 SpO2），无则填 HS_VALUE_INVALID */
} hs_sample_t;

typedef hs_status_t (*hs_init_fn_t)(void);
typedef hs_status_t (*hs_measure_fn_t)(hs_sample_t *out);

/**
 * @brief 传感器描述符：把一个测量项绑定到它的实现函数与负责人。
 */
typedef struct {
    hs_item_t        item;
    const char      *name;    /* 短名，用于 UI 与调试打印 */
    const char      *owner;   /* 负责人，与头文件 @owner 一致 */
    hs_init_fn_t     init;    /* 上电初始化，可为 NULL 表示无需初始化 */
    hs_measure_fn_t  measure; /* 采集一次，写入 hs_sample_t */
} hs_sensor_t;

/**
 * @brief 全局传感器注册表（在 app/app_sensors.c 中定义）。
 *        数组以 item==HS_ITEM_COUNT 或 measure==NULL 的哨兵项结尾。
 */
extern const hs_sensor_t g_hs_registry[];

/** @brief 返回状态码的可读名字，供调试串口打印。 */
const char *hs_status_str(hs_status_t s);

#ifdef __cplusplus
}
#endif

#endif /* HEALTH_IF_H */
