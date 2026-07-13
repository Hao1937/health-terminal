# =============================================================================
#  arm-none-eabi 交叉编译工具链文件（STM32F103C8T6 / Cortex-M3）
#  由 CMakePresets.json 的 stm32 preset 通过 toolchainFile 引用。
#  负责人：王宇浩（组长）
# =============================================================================
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# 允许用户用 -DTOOLCHAIN_PREFIX 覆盖，默认 arm-none-eabi-
if(NOT DEFINED TOOLCHAIN_PREFIX)
    set(TOOLCHAIN_PREFIX arm-none-eabi-)
endif()

# Windows 下可执行文件带 .exe 后缀
if(WIN32)
    set(TC_SUFFIX ".exe")
else()
    set(TC_SUFFIX "")
endif()

set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}gcc${TC_SUFFIX})
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc${TC_SUFFIX})
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++${TC_SUFFIX})
set(CMAKE_OBJCOPY      ${TOOLCHAIN_PREFIX}objcopy${TC_SUFFIX} CACHE INTERNAL "")
set(CMAKE_SIZE         ${TOOLCHAIN_PREFIX}size${TC_SUFFIX}    CACHE INTERNAL "")

# 交叉编译时编译器不能链接出可跑的宿主程序，用静态库探测跳过链接测试
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# 只在工具链目录找程序，不去系统路径找库/头文件
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# ---- Cortex-M3 目标架构标志 ----
set(CPU_FLAGS "-mcpu=cortex-m3 -mthumb")

# 公共编译标志：-Os 控制体积（F103C8 仅 64KB Flash），分节便于 --gc-sections 裁剪
set(CMAKE_C_FLAGS_INIT
    "${CPU_FLAGS} -Os -ffunction-sections -fdata-sections -Wall -Wextra -fno-common")
set(CMAKE_ASM_FLAGS_INIT "${CPU_FLAGS} -x assembler-with-cpp")

# 链接标志：使用 nano newlib（省体积）、垃圾回收未用节、生成 map 文件
set(CMAKE_EXE_LINKER_FLAGS_INIT
    "${CPU_FLAGS} --specs=nano.specs --specs=nosys.specs -Wl,--gc-sections -Wl,--print-memory-usage")
