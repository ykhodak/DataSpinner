
cmake_minimum_required(VERSION 3.20)

project("generated_code")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find requirements
find_package(fastcdr REQUIRED)
find_package(fastdds 3 REQUIRED)


message(STATUS "Configuring mddelta...")
add_library(mddelta_lib mddeltaTypeObjectSupport.cxx mddeltaPubSubTypes.cxx)
target_link_libraries(mddelta_lib fastcdr fastdds)

add_executable(fastdds_pubsub mddeltaApplication.cxx mddeltaPublisherApp.cxx mddeltaSubscriberApp.cxx mddeltamain.cxx md_dds_util.cpp)
set_property(TARGET fastdds_pubsub PROPERTY CXX_STANDARD 23)
target_include_directories(fastdds_pubsub PUBLIC . ../spinner_core)
target_link_libraries(fastdds_pubsub fastcdr fastdds
        mddelta_lib
        spinner_core
        )



