include(CMakePrintHelpers)

set(SRC util.h util.cpp
        socket_buffer.h socket_buffer.cpp
        archive.h archive.cpp
        tcp_connection.h tcp_connection.cpp
        udp_connection.h udp_connection.cpp
        admin_connection.h admin_connection.cpp
        simulator.h simulator.cpp
        ref_table.h ref_table.cpp        
       )

add_library(socket_transport STATIC ${SRC})
set_property(TARGET socket_transport PROPERTY CXX_STANDARD 23)
target_include_directories(socket_transport PUBLIC . spinner_core)
target_link_libraries(socket_transport PRIVATE libevent::core)

add_executable (tcp_publisher tcp_publisher.cpp)
add_executable (tcp_subscriber tcp_subscriber.cpp)
add_executable (admin_client admin_client.cpp)
add_executable (udp_publisher udp_publisher.cpp)
add_executable (udp_subscriber udp_subscriber.cpp)


set_property(TARGET tcp_publisher PROPERTY CXX_STANDARD 23)
set_property(TARGET tcp_subscriber PROPERTY CXX_STANDARD 23)
set_property(TARGET admin_client PROPERTY CXX_STANDARD 23)
set_property(TARGET udp_publisher PROPERTY CXX_STANDARD 23)
set_property(TARGET udp_subscriber PROPERTY CXX_STANDARD 23)


target_link_libraries(tcp_publisher PRIVATE libevent::core socket_transport spinner_core)
target_link_libraries(tcp_subscriber PRIVATE libevent::core socket_transport spinner_core)
target_link_libraries(admin_client PRIVATE libevent::core socket_transport spinner_core)
target_link_libraries(udp_publisher PRIVATE libevent::core socket_transport spinner_core)
target_link_libraries(udp_subscriber PRIVATE libevent::core socket_transport spinner_core)

if(UNIX)
        target_link_libraries(tcp_publisher PRIVATE libevent::pthreads)
        target_link_libraries(tcp_subscriber PRIVATE libevent::pthreads)
        target_link_libraries(admin_client PRIVATE libevent::pthreads)
        target_link_libraries(udp_publisher PRIVATE libevent::pthreads)
        target_link_libraries(udp_subscriber PRIVATE libevent::pthreads)
endif()
#Threads::Threads
#cmake_print_properties(TARGETS libevent::pthreads PROPERTIES INTERFACE_INCLUDE_DIRECTORIES INTERFACE_LINK_LIBRARIES)
