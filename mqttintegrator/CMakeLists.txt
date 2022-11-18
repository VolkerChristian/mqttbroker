cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json 3.7.0)

if(NOT SNODEC_INTREE_BUILD)
    find_package(
        snodec COMPONENTS net-in-stream-legacy net-in-stream-tls
                          net-un-stream-legacy mqtt-client
    )
endif(NOT SNODEC_INTREE_BUILD)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(ADDITIONAL_OPTIONS
        -Weverything
        -Wno-c++98-compat
        -Wno-c++98-compat-bind-to-temporary-copy
        -Wno-exit-time-destructors
        -Wno-global-constructors
        -Wno-padded
        -Wno-shadow
        -Wno-used-but-marked-unused
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # using GCC
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    # using Intel C++
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # using Visual Studio C++
endif()

add_compile_options(
    -Werror
    -Wall
    -Wextra
    -Wno-psabi # needed for RaspberryPi
    -Wconversion
    -Wpedantic
    -Wconversion
    -Wuninitialized
    -Wunreachable-code
    -Wno-float-equal
    -Wno-implicit-int-float-conversion
    -pedantic-errors
    -fexec-charset=UTF-8
    ${ADDITIONAL_OPTIONS}
)

set(MQTTINTEGRATOR_CPP mqttintegrator.cpp SocketContext.cpp)

set(MQTTINTEGRATOR_H SocketContextFactory.h SocketContextFactory.hpp)

add_executable(mqttintegrator ${MQTTINTEGRATOR_CPP} ${MQTTINTEGRATOR_H})

target_include_directories(
    mqttintegrator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                          ${CMAKE_CURRENT_BINARY_DIR} ${PROJECT_SOURCE_DIR}
)
target_link_libraries(
    mqttintegrator
    PUBLIC snodec::mqtt-client snodec::net-in-stream-legacy
           snodec::net-in-stream-tls snodec::net-un-stream-legacy
    PRIVATE mqtt-mapping
)

if(SNODEC_INTREE_BUILD)
    set_target_properties(
        mqttintegrator PROPERTIES INSTALL_RPATH "${SNODEC_IOT_MQTT_RPATH}"
    )
endif(SNODEC_INTREE_BUILD)

install(TARGETS mqttintegrator RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
