cmake_minimum_required(VERSION 2.8)
project (grapevine)

set(CMAKE_C_COMPILER "/usr/bin/clang")

#set(CMAKE_SUPPRESS_REGENERATION TRUE)

set(GV_DEBUG_FLAGS "-DGV_MAIN -DGV_DEBUG=20")
set(GV_CLANG_FLAGS "-Weverything -Wno-variadic-macros -Wno-gnu-zero-variadic-macro-arguments")

#set(CMAKE_C_FLAGS "-Weverything -Wno-variadic-macros -Wno-gnu-zero-variadic-macro-arguments")

set(c-sources
        grapevine.c
        gv_util.c
        gv_type.c)

set(c-sources-test
        test_gv_util.c)

set(c-headers-test
        gv_util.h)

if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "DebugTest")
    message(Building Debug)
    list(APPEND CMAKE_C_FLAGS ${GV_DEBUG_FLAGS})
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Test" OR CMAKE_BUILD_TYPE STREQUAL "DebugTest")
    message(Building Test)
    foreach(source ${c-sources-test})
        list(APPEND sources ${CMAKE_CURRENT_SOURCE_DIR}/test/${source})
    endforeach()
    foreach(source ${c-headers-test})
        list(APPEND sources ${CMAKE_CURRENT_SOURCE_DIR}/src/${source})
    endforeach()
    list(APPEND GV_DEBUG_FLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}/src}")
endif()

if (CMAKE_COMPILER_ID MATCHES "Clang")
    list(APPEND CMAKE_C_FLAGS ${GV_CLANG_FLAGS})
endif()

set(GV_CMAKE_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(GV_CMAKE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

include(CheckIncludeFiles)
check_include_files(zmq.h GV_HAVE_ZMQ)


foreach(source ${c-sources})
    list(APPEND sources ${CMAKE_CURRENT_SOURCE_DIR}/src/${source})
endforeach()

set(public_headers include/grapevine.h)
set(private_headers ${CMAKE_CURRENT_SOURCE_DIR}/src/gv_util.h)

if (CMAKE_BUILD_TYPE STREQUAL "Test" OR CMAKE_BUILD_TYPE STREQUAL "DebugTest")
    add_executable(grapevine ${sources})
else()
    add_executable(grapevine ${sources})
endif()
