/**
 * @file    test_util.h
 * @brief   极简单元测试宏（不引入第三方框架，保持零依赖）。
 * @owner   王宇浩（组长）
 *
 * 每个测试可执行文件 include 本头，用 CHECK/CHECK_EQ 断言，main 末尾 return
 * TEST_SUMMARY()（有失败返回非 0，ctest 据此判定）。
 */
#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <stdio.h>

static int g_test_pass = 0;
static int g_test_fail = 0;

#define CHECK(cond) do { \
    if (cond) { g_test_pass++; } \
    else { g_test_fail++; \
        printf("  [FAIL] %s:%d  CHECK(%s)\n", __FILE__, __LINE__, #cond); } \
} while (0)

#define CHECK_EQ(a, b) do { \
    long long _a = (long long)(a), _b = (long long)(b); \
    if (_a == _b) { g_test_pass++; } \
    else { g_test_fail++; \
        printf("  [FAIL] %s:%d  CHECK_EQ(%s, %s)  got %lld != %lld\n", \
               __FILE__, __LINE__, #a, #b, _a, _b); } \
} while (0)

/* 允许误差的近似相等（整数量纲） */
#define CHECK_NEAR(a, b, tol) do { \
    long long _a = (long long)(a), _b = (long long)(b), _t = (long long)(tol); \
    long long _d = _a > _b ? _a - _b : _b - _a; \
    if (_d <= _t) { g_test_pass++; } \
    else { g_test_fail++; \
        printf("  [FAIL] %s:%d  CHECK_NEAR(%s,%s,%s)  |%lld-%lld|=%lld > %lld\n", \
               __FILE__, __LINE__, #a, #b, #tol, _a, _b, _d, _t); } \
} while (0)

#define TEST_SUMMARY() ( \
    printf("  %d passed, %d failed\n", g_test_pass, g_test_fail), \
    (g_test_fail == 0) ? 0 : 1 )

#endif /* TEST_UTIL_H */
