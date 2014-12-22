cmake_minimum_required(VERSION 2.8)
project (grapevine)

set(CMAKE_C_COMPILER "/usr/bin/clang")

#set(CMAKE_SUPPRESS_REGENERATION TRUE)

set(CMAKE_C_FLAGS "-DGV_DEBUG -Weverything -Wno-variadic-macros -Wno-gnu-zero-variadic-macro-arguments")

set(GV_CMAKE_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(GV_CMAKE_INCLUDE_DIR "${CMAKE_CURRENT_SURCE_DIR}/include")

include(CheckIncludeFiles)
check_include_files(zmq.h GV_HAVE_ZMQ)

set(c-sources
        grapevine.c
        gv_util.c
        gv_type.c)

foreach(source ${c-sources})
    list(APPEND sources ${CMAKE_CURRENT_SOURCE_DIR}/src/${source})
endforeach()

include_directories(include)
set(public_headers include/grapevine.h)
#add_subdirectory(src)
#add_subdirectory("include")

add_executable(grapevine ${sources})
