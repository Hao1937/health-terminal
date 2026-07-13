cmake_minimum_required(VERSION 3.21)

# =============================================================================
#  health-terminal —— 多维度智能健康体测终端（STM32F103C8T6）
#
#  两种构建入口（见 CMakePresets.json）：
#    - host     : 在 PC 上编译 algorithms/ 并跑 tests/ 单元测试
#    - firmware : arm-none-eabi 交叉编译出 firmware.elf/.bin/.hex
#
#  用 -DHT_BUILD=host|firmware 选择（preset 已设好）。
# =============================================================================

set(HT_BUILD "host" CACHE STRING "构建目标：host 或 firmware")
set_property(CACHE HT_BUILD PROPERTY STRINGS host firmware)

if(HT_BUILD STREQUAL "firmware")
    # 交叉编译固件：语言含汇编（启动文件）
    project(health-terminal C ASM)
else()
    project(health-terminal C)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 源码含 UTF-8 中文注释：MSVC 默认按系统代码页(GBK)解析会报 C4819，显式指定 UTF-8
if(MSVC)
    add_compile_options(/utf-8)
endif()

# 公共头（契约）：include/ 下的 health_if.h / health_record.h
set(HT_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
set(HT_ALGO_DIR    "${CMAKE_SOURCE_DIR}/algorithms")

# algorithms/ 为纯 C、无硬件依赖，host 与 firmware 都用它
set(HT_ALGO_SOURCES
    ${HT_ALGO_DIR}/crc16.c
    ${HT_ALGO_DIR}/filters.c
    ${HT_ALGO_DIR}/record_codec.c
    ${HT_ALGO_DIR}/spo2_hr.c
    ${HT_ALGO_DIR}/balance.c
    ${HT_ALGO_DIR}/bodyfat_navy.c
    ${HT_ALGO_DIR}/health_score.c
)

add_library(health_algo STATIC ${HT_ALGO_SOURCES})
target_include_directories(health_algo PUBLIC ${HT_INCLUDE_DIR} ${HT_ALGO_DIR})

if(HT_BUILD STREQUAL "firmware")
    # ---------------------------------------------------------------------
    #  固件构建
    # ---------------------------------------------------------------------
    include(${CMAKE_SOURCE_DIR}/cmake/firmware.cmake)
else()
    # ---------------------------------------------------------------------
    #  Host 单元测试
    # ---------------------------------------------------------------------
    enable_testing()
    add_subdirectory(tests)
endif()
