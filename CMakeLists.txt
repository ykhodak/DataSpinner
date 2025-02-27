
cmake_minimum_required (VERSION 3.24)
if (WIN32)
	set(CMAKE_TOOLCHAIN_FILE "c:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake")
endif (WIN32)

#set(CMAKE_BUILD_TYPE Debug)
if(UNIX)
    if(NOT CMAKE_BUILD_TYPE)
           set(CMAKE_BUILD_TYPE Release)
    endif()

    set(CMAKE_CXX_FLAGS "-Wall -Wextra")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project (spinner C CXX)

find_package(benchmark REQUIRED)
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)
find_package(Libevent REQUIRED)
#find_package(spdlog CONFIG REQUIRED)

message(STATUS "Path to GTest: ${benchmark_DIR}")
message(STATUS "Path to GTest: ${GTest_DIR}")
message(STATUS "Path to Threads: ${Threads_DIR}")
message(STATUS "Path to Libevent: ${Libevent_DIR}")

add_subdirectory(src/spinner_core)
add_subdirectory(src)
add_subdirectory(tests)
if(DEFINED ENV{SPINNER_NO_DDS})
  message(STATUS "skip DDS. SPINNER_NO_DDS is defined: $ENV{SPINNER_NO_DDS}")
else()
  add_subdirectory(src/dds)
endif()


