# host 单元测试：每个 test_*.c 编译成一个可执行文件并注册到 ctest
# 负责人：王宇浩（组长）

set(HT_TESTS
    test_filters
    test_codec
    test_algorithms
)

foreach(t ${HT_TESTS})
    add_executable(${t} ${t}.c)
    target_include_directories(${t} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${t} PRIVATE health_algo)
    # 数学库（互补滤波/体脂率用到 <math.h>），Windows/MSVC 无需显式链接
    if(NOT MSVC)
        target_link_libraries(${t} PRIVATE m)
    endif()
    add_test(NAME ${t} COMMAND ${t})
endforeach()
