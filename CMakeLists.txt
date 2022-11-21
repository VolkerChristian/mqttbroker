cmake_minimum_required(VERSION 3.5)

project(MQTTBroker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(lib)
add_subdirectory(mqttbroker)
add_subdirectory(mqttintegrator)
add_subdirectory(mqttwebfrontend)
