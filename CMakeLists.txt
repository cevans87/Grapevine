cmake_minimum_required(VERSION 2.8)
project (grapevine)

if (NOT GV_COMPILER)
    set(GV_COMPILER "clang++")
endif()

if (NOT GV_BUILD)
    set(GV_BUILD "release")
endif()

set(CMAKE_CXX_COMPILER "/usr/bin/${GV_COMPILER}")
message("Using compiler ${CMAKE_CXX_COMPILER}")

#set(CMAKE_SUPPRESS_REGENERATION TRUE)

##############################################################################
# Source Files
##############################################################################
set(c-sources
        grapevine.cpp
        gv_util.cpp
        gv_type.cpp)

set(c-sources-test
        test_gv_util.cpp)

#set(c-headers-test
#        gv_util.h)

##############################################################################
# Definitions and Compiler Warnings
##############################################################################

#add_definitions(-DGV_MAIN)
add_definitions(-DGV_DEBUG=20)

# Clang
if (GV_COMPILER STREQUAL "clang++")
    set(GV_CLANG_FLAGS "${GV_CLANG_FLAGS} --std=c++11")
    set(GV_CLANG_FLAGS "${GV_CLANG_FLAGS} -Weverything")
    set(GV_CLANG_FLAGS "${GV_CLANG_FLAGS} -Wno-variadic-macros")
    set(GV_CLANG_FLAGS "${GV_CLANG_FLAGS} -Wno-gnu-zero-variadic-macro-arguments")
    set(GV_CLANG_FLAGS "${GV_CLANG_FLAGS} -Wno-c++98-compat-pedantic")
    list(APPEND CMAKE_CXX_FLAGS ${GV_CLANG_FLAGS})
endif()

##############################################################################
# Build Type
# Build types are Release, Debug, Test, DebugTest
# Ex: In build folder, cmake -DCMAKE_BUILD_TYPE=Debug ..
##############################################################################
if (GV_BUILD STREQUAL "Debug" OR GV_BUILD STREQUAL "DebugTest")
    message("Building Debug")
    list(APPEND CMAKE_CXX_FLAGS ${GV_DEBUG_FLAGS})
endif()

if (GV_BUILD STREQUAL "Test" OR GV_BUILD STREQUAL "DebugTest")
    message("Building Test")
    foreach(source ${c-sources-test})
        list(APPEND sources-test ${CMAKE_CURRENT_SOURCE_DIR}/test/${source})
    endforeach()
    list(APPEND CMAKE_CXX_FLAGS ${GV_DEBUG_FLAGS})
endif()

##############################################################################
# Includes
##############################################################################
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/test/)

#set(GV_CMAKE_INCLUDE_DIRS "${GV_CMAKE_INCLUDE_DIRS} -I${CMAKE_CURRENT_SOURCE_DIR}/include")
#set(GV_CMAKE_INCLUDE_DIRS "${GV_CMAKE_INCLUDE_DIRS} -I${CMAKE_CURRENT_SOURCE_DIR}/src")
message(cflags: ${CMAKE_CXX_FLAGS})

include(CheckIncludeFiles)
check_include_files(zmq.h GV_HAVE_ZMQ)

foreach(source ${c-sources})
    list(APPEND sources ${CMAKE_CURRENT_SOURCE_DIR}/src/${source})
endforeach()

set(public_headers include/grapevine.h)
set(private_headers ${CMAKE_CURRENT_SOURCE_DIR}/src/gv_util.h)

if (CMAKE_BUILD_TYPE STREQUAL "Test" OR CMAKE_BUILD_TYPE STREQUAL "DebugTest")
    add_executable(test_grapevine ${sources} ${sources-test})
else()
    add_library(grapevine ${sources})
endif()
