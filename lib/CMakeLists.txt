cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT SNODEC_INTREE_BUILD)
    find_package(
        snodec COMPONENTS mqtt
    )
endif(NOT SNODEC_INTREE_BUILD)

add_library(
    mqtt-mapping STATIC JsonMappingReader.cpp MqttMapper.cpp
                        JsonMappingReader.h MqttMapper.h
)

target_link_libraries(mqtt-mapping snodec::mqtt)
