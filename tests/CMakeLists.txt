find_package(GTest REQUIRED)
find_package(benchmark REQUIRED)

add_executable(TEST_msg test_msg.cpp)
add_executable(TEST_ref test_ref.cpp test_tu.h test_tu.cpp)
add_executable(BM_msg bm_msg.cpp test_tu.h test_tu.cpp)
add_executable(BM_util bm_util.cpp test_tu.h test_tu.cpp)
add_executable(BM_ref bm_ref.cpp test_tu.h test_tu.cpp)

target_link_libraries(TEST_msg PRIVATE  GTest::gtest_main socket_transport spinner_core)
target_link_libraries(TEST_ref PRIVATE  GTest::gtest_main socket_transport spinner_core)
target_link_libraries(BM_msg PRIVATE  benchmark::benchmark socket_transport spinner_core libevent::core)
target_link_libraries(BM_util PRIVATE  benchmark::benchmark socket_transport spinner_core libevent::core)
target_link_libraries(BM_ref PRIVATE  benchmark::benchmark socket_transport spinner_core libevent::core)
#target_link_libraries(bm_util PRIVATE  benchmark::benchmark socket_transport spinner_core)


#libevent::extra

if(UNIX)
    target_link_libraries(TEST_ref PRIVATE libevent::pthreads)
    target_link_libraries(TEST_msg PRIVATE libevent::pthreads)
    target_link_libraries(BM_msg PRIVATE libevent::pthreads)
    target_link_libraries(BM_util PRIVATE libevent::pthreads)
    target_link_libraries(BM_ref PRIVATE libevent::pthreads)
endif()

set_property(TARGET TEST_ref PROPERTY CXX_STANDARD 23)
set_property(TARGET TEST_msg PROPERTY CXX_STANDARD 23)
set_property(TARGET BM_msg PROPERTY CXX_STANDARD 23)
set_property(TARGET BM_util PROPERTY CXX_STANDARD 23)
set_property(TARGET BM_ref PROPERTY CXX_STANDARD 23)

