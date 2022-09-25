cmake_minimum_required(VERSION 3.5)

project(MQTTBroker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(snodec COMPONENTS net-in-stream-legacy mqtt)

set(MQTTBROKER_CPP
    mqttbroker.cpp broker/Broker.cpp broker/SocketContext.cpp
    broker/SocketContextFactory.cpp broker/SubscriberTree.cpp
    broker/TopicTree.cpp
)
set(MQTTBROKER_H
    broker/Broker.h broker/SocketContext.h broker/SocketContextFactory.h
    broker/SubscriberTree.h broker/TopicTree.h
)

add_executable(mqttbroker ${MQTTBROKER_CPP} ${MQTTBROKER_H})

target_link_libraries(
    mqttbroker PRIVATE snodec::net-in-stream-legacy snodec::mqtt
)

target_include_directories(mqttbroker PUBLIC ${PROJECT_SOURCE_DIR})

set_target_properties(mqttbroker PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)

install(TARGETS mqttbroker RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
