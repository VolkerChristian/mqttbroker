cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#configure_file(config.h.in config.h @ONLY)

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

set(CLIENTBROKER_CPP clientbroker.cpp SocketContext.cpp ../JsonMappingReader.cpp)

set(CLIENTBROKER_H SocketContextFactory.h SocketContextFactory.hpp ../JsonMappingReader.h)

add_executable(clientbroker ${CLIENTBROKER_CPP} ${CLIENTBROKER_H})

target_include_directories(
    clientbroker PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
)

if(SNODEC_INTREE_BUILD)
    set_target_properties(
        clientbroker PROPERTIES INSTALL_RPATH "${SNODEC_IOT_MQTT_RPATH}"
    )
endif(SNODEC_INTREE_BUILD)

target_link_libraries(
    clientbroker PUBLIC snodec::mqtt-client snodec::net-in-stream-legacy
                        snodec::net-in-stream-tls snodec::net-un-stream-legacy
)

install(TARGETS clientbroker RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
