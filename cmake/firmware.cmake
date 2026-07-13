# =============================================================================
#  firmware.cmake —— STM32F103C8T6 固件构建
#  由顶层 CMakeLists.txt 在 HT_BUILD=firmware 时 include。
#  负责人：王宇浩（组长）
# =============================================================================

# ---- 1. 编译开关 MODULE_SET：决定哪些人的模块编译为真实现，其余为桩 --------
# 每个模块的 <mod>.c 都恒被编译；MODULE_ENABLED_<NAME> 宏在其内部切换
# 「真实现 / 桩(HS_NOT_IMPLEMENTED)」两个分支，从而不产生符号重复，app 恒可链接。
set(MODULE_SET "full" CACHE STRING "模块子集：yuhao|chafanting|liuyanming|full")
set_property(CACHE MODULE_SET PROPERTY STRINGS yuhao chafanting liuyanming full)

set(ALL_MODULES
    max30102 hx711_weight hx711_grip hcsr04 ds18b20 mpu6050
    oled keypad reaction ble storage)

if(MODULE_SET STREQUAL "yuhao")
    set(ENABLED_MODULES max30102 hx711_grip)
elseif(MODULE_SET STREQUAL "chafanting")
    set(ENABLED_MODULES hx711_weight hcsr04 ds18b20)
elseif(MODULE_SET STREQUAL "liuyanming")
    set(ENABLED_MODULES oled keypad reaction ble storage mpu6050)
elseif(MODULE_SET STREQUAL "full")
    set(ENABLED_MODULES ${ALL_MODULES})
else()
    message(FATAL_ERROR "未知 MODULE_SET=${MODULE_SET}，应为 yuhao|chafanting|liuyanming|full")
endif()
message(STATUS "MODULE_SET=${MODULE_SET} -> 真实现模块: ${ENABLED_MODULES}")

# ---- 2. STM32CubeF1 HAL（scripts/fetch_hal.sh 按需 sparse-clone 到此路径） ------
set(STM32CUBEF1_DIR "${CMAKE_SOURCE_DIR}/core/STM32CubeF1"
    CACHE PATH "STM32CubeF1 HAL 根目录（由 scripts/fetch_hal.sh 获取）")
if(NOT EXISTS "${STM32CUBEF1_DIR}/Drivers/STM32F1xx_HAL_Driver")
    message(FATAL_ERROR
        "未找到 STM32CubeF1 HAL：${STM32CUBEF1_DIR}\n"
        "请先获取 HAL（只拉 Drivers，约数十 MB）：\n"
        "    bash scripts/fetch_hal.sh\n"
        "或运行 scripts/bootstrap.sh（Linux/macOS）/ scripts/bootstrap.ps1（Windows），会自动获取。\n"
        "详见 docs/开发环境搭建.md。")
endif()

set(HAL_DRV "${STM32CUBEF1_DIR}/Drivers/STM32F1xx_HAL_Driver")
set(CMSIS   "${STM32CUBEF1_DIR}/Drivers/CMSIS")

# 只挑本项目用到的 HAL 模块，控制体积（避免全量编译撑爆 64KB Flash）
set(HAL_SOURCES
    ${HAL_DRV}/Src/stm32f1xx_hal.c
    ${HAL_DRV}/Src/stm32f1xx_hal_cortex.c
    ${HAL_DRV}/Src/stm32f1xx_hal_rcc.c
    ${HAL_DRV}/Src/stm32f1xx_hal_gpio.c
    ${HAL_DRV}/Src/stm32f1xx_hal_dma.c
    ${HAL_DRV}/Src/stm32f1xx_hal_i2c.c
    ${HAL_DRV}/Src/stm32f1xx_hal_uart.c
    ${HAL_DRV}/Src/stm32f1xx_hal_tim.c
    ${HAL_DRV}/Src/stm32f1xx_hal_flash.c
    ${HAL_DRV}/Src/stm32f1xx_hal_flash_ex.c
)

# ---- 3. 工程自身源码 --------------------------------------------------------
file(GLOB BSP_SOURCES    ${CMAKE_SOURCE_DIR}/bsp/*.c)
file(GLOB APP_SOURCES    ${CMAKE_SOURCE_DIR}/app/*.c)
file(GLOB CORE_SOURCES   ${CMAKE_SOURCE_DIR}/core/src/*.c)
set(MODULE_SOURCES "")
foreach(m ${ALL_MODULES})
    list(APPEND MODULE_SOURCES ${CMAKE_SOURCE_DIR}/modules/${m}/${m}.c)
endforeach()

# 启动文件与 system_stm32f1xx.c 直接采用 CubeF1 官方模板（避免手写汇编出错）。
# 链接脚本用工程自带版本：为片内 Flash 模拟 EEPROM 预留了最后 4 页。
set(STARTUP ${CMSIS}/Device/ST/STM32F1xx/Source/Templates/gcc/startup_stm32f103xb.s)
list(APPEND CORE_SOURCES
     ${CMSIS}/Device/ST/STM32F1xx/Source/Templates/system_stm32f1xx.c)
set(LDSCRIPT ${CMAKE_SOURCE_DIR}/core/ldscripts/STM32F103C8Tx_FLASH.ld)

# ---- 4. 目标 ----------------------------------------------------------------
add_executable(firmware
    ${STARTUP}
    ${CORE_SOURCES}
    ${HAL_SOURCES}
    ${BSP_SOURCES}
    ${MODULE_SOURCES}
    ${APP_SOURCES}
)

target_include_directories(firmware PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/algorithms
    ${CMAKE_SOURCE_DIR}/core/include
    ${CMAKE_SOURCE_DIR}/bsp
    ${CMAKE_SOURCE_DIR}/modules
    ${HAL_DRV}/Inc
    ${CMSIS}/Device/ST/STM32F1xx/Include
    ${CMSIS}/Include
)

target_link_libraries(firmware PRIVATE health_algo)

target_compile_definitions(firmware PRIVATE
    STM32F103xB
    USE_HAL_DRIVER
    HSE_VALUE=8000000        # 最小系统板 8MHz 晶振
)
# 逐模块打开「真实现」分支
foreach(m ${ENABLED_MODULES})
    string(TOUPPER ${m} M_UP)
    target_compile_definitions(firmware PRIVATE MODULE_ENABLED_${M_UP}=1)
endforeach()

target_link_options(firmware PRIVATE
    -T${LDSCRIPT}
    -Wl,-Map=${CMAKE_BINARY_DIR}/firmware.map
)
set_target_properties(firmware PROPERTIES
    OUTPUT_NAME "firmware"
    SUFFIX ".elf"
    LINK_DEPENDS ${LDSCRIPT})

# ---- 5. 生成 .bin / .hex 并打印体积 -----------------------------------------
add_custom_command(TARGET firmware POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:firmware> ${CMAKE_BINARY_DIR}/firmware.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:firmware> ${CMAKE_BINARY_DIR}/firmware.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:firmware>
    COMMENT "生成 firmware.bin / firmware.hex 并统计 Flash/RAM 占用")
