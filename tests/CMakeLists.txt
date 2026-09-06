# host 单元测试：每个 test_*.c 编译成一个可执行文件并注册到 ctest
# 负责人：王宇浩（组长）

set(HT_TESTS
    test_filters
    test_codec
    test_algorithms
    test_height
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

# 假数据模块回归测试：模拟状态机每轮 measure 前都会调用 init 的真实流程。
add_executable(test_hx711_grip_demo
    test_hx711_grip_demo.c
    ${CMAKE_SOURCE_DIR}/modules/hx711_grip/hx711_grip.c
)
target_include_directories(test_hx711_grip_demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/modules/hx711_grip
)
target_compile_definitions(test_hx711_grip_demo PRIVATE
    MODULE_ENABLED_HX711_GRIP=1
    GRIP_DEMO_MODE=1
)
target_link_libraries(test_hx711_grip_demo PRIVATE health_algo)
add_test(NAME test_hx711_grip_demo COMMAND test_hx711_grip_demo)
