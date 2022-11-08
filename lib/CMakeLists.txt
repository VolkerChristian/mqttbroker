cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(
    mqtt-mapping STATIC JsonMappingReader.cpp MqttMapper.cpp
                        JsonMappingReader.h MqttMapper.h
)

target_link_libraries(mqtt-mapping snodec::mqtt)
