cmake_minimum_required(VERSION 3.13)
project(tinyc CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Bison
find_program(BISON_EXECUTABLE bison)
if(NOT BISON_EXECUTABLE)
    message(FATAL_ERROR "bison not found")
endif()

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.hpp
    COMMAND ${BISON_EXECUTABLE} -d ${CMAKE_CURRENT_SOURCE_DIR}/parser.y
            -o ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.cpp
            --defines=${CMAKE_CURRENT_BINARY_DIR}/parser.tab.hpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/parser.y
)

# Flex
find_program(FLEX_EXECUTABLE flex)
if(NOT FLEX_EXECUTABLE)
    message(FATAL_ERROR "flex not found")
endif()

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/scanner.yy.cpp
    COMMAND ${FLEX_EXECUTABLE} -o ${CMAKE_CURRENT_BINARY_DIR}/scanner.yy.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/scanner.l
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/scanner.l
)

add_executable(tinyc
    main.cpp
    ast.cpp
    sema.cpp
    codegen.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/scanner.yy.cpp
)

target_include_directories(tinyc PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
llvm_map_components_to_libnames(LLVM_LIBS core support)
target_link_libraries(tinyc ${LLVM_LIBS})
