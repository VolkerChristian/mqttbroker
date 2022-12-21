cmake_minimum_required(VERSION 3.5)

find_package(snodec COMPONENTS websocket-client)

find_package(nlohmann_json)

get_target_property(
    WEBSOCKET_SUBPROTOCOL_CLIENT_INSTALL_LIBDIR snodec::websocket-client
    WEBSOCKET_SUBPROTOCOL_CLIENT_INSTALL_LIBDIR
)

get_target_property(
    WEBSOCKET_SUBPROTOCOL_CLIENT_INCLUDE_DIRS snodec::websocket-client
    WEBSOCKET_SUBPROTOCOL_CLIENT_INCLUDE_DIRS
)

set(MQTTCLIENTSUBPROTOCOL_CPP SubProtocolFactory.cpp)

set(MQTTCLIENTSUBPROTOCOL_H SubProtocolFactory.h)

add_library(
    mqttclientsubprotocol SHARED ${MQTTCLIENTSUBPROTOCOL_CPP}
                                 ${MQTTCLIENTSUBPROTOCOL_H}
)

target_link_libraries(
    mqttclientsubprotocol PUBLIC mqtt-integrator nlohmann_json::nlohmann_json
)

target_include_directories(
    mqttclientsubprotocol PUBLIC ${WEBSOCKET_SUBPROTOCOL_CLIENT_INCLUDE_DIRS}
                                 ${PROJECT_SOURCE_DIR}
)

set_target_properties(
    mqttclientsubprotocol PROPERTIES OUTPUT_NAME "snodec-websocket-mqtt"
                                     SOVERSION 1 # PREFIX "ssp"
)

install(TARGETS mqttclientsubprotocol
        LIBRARY DESTINATION ${WEBSOCKET_SUBPROTOCOL_CLIENT_INSTALL_LIBDIR}
)
