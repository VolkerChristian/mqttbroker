cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json 3.7.0)

add_subdirectory(json-schema-validator)

if(NOT SNODEC_INTREE_BUILD)
    find_package(snodec COMPONENTS mqtt)
endif(NOT SNODEC_INTREE_BUILD)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(ADDITIONAL_OPTIONS
        -Weverything
        -Wno-c++98-compat
        -Wno-exit-time-destructors
        -Wno-global-constructors
        -Wno-padded
        -Wno-shadow
        -Wno-shadow-field
        -Wno-used-but-marked-unused
        -Wno-weak-vtables
        -Wno-reserved-macro-identifier
        -Wno-c++98-compat-pedantic
        -Wno-covered-switch-default
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
    -Wno-switch-enum
    -Wno-implicit-int-float-conversion
    -pedantic-errors
    -fexec-charset=UTF-8
    ${ADDITIONAL_OPTIONS}
)

function(make_includable input_file output_file)
    message(Recreating ${output_file})
    file(READ ${input_file} content)
    set(bdelim "(")
    set(edelim ")")
    set(content "R\"${bdelim}\n${content}${edelim}\"\n")
    file(WRITE ${output_file} "${content}")
endfunction(make_includable)

make_includable(mapping-schema.json ${CMAKE_CURRENT_BINARY_DIR}/mapping-schema.json.h)

add_library(
    mqtt-mapping STATIC JsonMappingReader.cpp MqttMapper.cpp
                        JsonMappingReader.h MqttMapper.h mapping-schema.json.h
)

target_include_directories(mqtt-mapping PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(mqtt-mapping PRIVATE snodec::mqtt nlohmann_json_schema_validator)
