cmake_minimum_required(VERSION 3.5)

find_package(snodec COMPONENTS websocket-client)

get_target_property(
    WEBSOCKET_SUBPROTOCOL_CLIENT_INSTALL_LIBDIR snodec::websocket-client
    WEBSOCKET_SUBPROTOCOL_CLIENT_INSTALL_LIBDIR
)

get_target_property(
    WEBSOCKET_SUBPROTOCOL_CLIENT_INCLUDE_DIRS snodec::websocket-client
    WEBSOCKET_SUBPROTOCOL_CLIENT_INCLUDE_DIRS
)

set(MQTTCLIENTSUBPROTOCOL_CPP Echo.cpp EchoFactory.cpp)

set(MQTTCLIENTSUBPROTOCOL_H Echo.h EchoFactory.h)

add_library(
    mqttclientsubprotocol SHARED ${MQTTCLIENTSUBPROTOCOL_CPP}
                                 ${MQTTCLIENTSUBPROTOCOL_H}
)

target_include_directories(
    mqttclientsubprotocol PUBLIC ${WEBSOCKET_SUBPROTOCOL_CLIENT_INCLUDE_DIRS}
)

set_target_properties(
    mqttclientsubprotocol
    PROPERTIES OUTPUT_NAME "snodec-websocket-mqtt"
               SOVERSION 1 # PREFIX "ssp"
)

install(TARGETS mqttclientsubprotocol
        LIBRARY DESTINATION ${WEBSOCKET_SUBPROTOCOL_CLIENT_INSTALL_LIBDIR}
)

if(CHECK_INCLUDES)
    set_property(
        TARGET mqttclientsubprotocol PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                                              ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)
