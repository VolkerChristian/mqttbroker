cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(snodec COMPONENTS mqtt-client)

add_library(mqtt-integrator SHARED Mqtt.cpp Mqtt.h)

target_include_directories(
    mqtt-integrator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                           ${CMAKE_CURRENT_BINARY_DIR} ${PROJECT_SOURCE_DIR}
)

target_link_libraries(mqtt-integrator PUBLIC snodec::mqtt-client mqtt-mapping)

install(TARGETS mqtt-integrator RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR})
