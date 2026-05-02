include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)

# 创建一个测试可执行文件
add_executable(test_ring_pool
    test_ring_packet_pool.cc                    
    ${CMAKE_SOURCE_DIR}/src/ring_packet_pool.cpp 
)

# 链接 Google Test 库和线程库
target_link_libraries(test_ring_pool
    gtest_main       # Google Test 提供 main 函数
    pthread          # Linux 下线程库
)

# 注册到 CTest，这样就可以用 ctest 命令统一跑测试
add_test(NAME RingPacketPoolTest COMMAND test_ring_pool)