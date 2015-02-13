cmake_minimum_required(VERSION 2.8)
project (grapevine)

# FIXME cevans87: stop forcing clang++, just prefer it.
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
        gv_browser.cpp
        gv_util.cpp
        gv_type.cpp)

set(c-sources-test
        gv_test.cpp
        test_gv_util.cpp
        test_gv_browser.cpp
        test_gv_type.cpp)

foreach(source ${c-sources})
    list(APPEND sources ${CMAKE_CURRENT_SOURCE_DIR}/src/${source})
endforeach()

foreach(source ${c-sources-test})
    list(APPEND sources-test ${CMAKE_CURRENT_SOURCE_DIR}/test/${source})
endforeach()

##############################################################################
# Compiler Flags
##############################################################################

set(GV_FLAGS "${GV_FLAGS} -fPIC")
if (GV_COMPILER STREQUAL "clang++")
    set(GV_FLAGS "${GV_FLAGS} --std=c++11")
    set(GV_FLAGS "${GV_FLAGS} --stdlib=libc++")
endif()

##############################################################################
# Compiler Warnings
##############################################################################
# Clang
if (GV_COMPILER STREQUAL "clang++")
    # FIXME cevans87: Most of these warnings are only annoying in one place,
    # and might be useful in the future. Use pragma directive where
    # appropriate.
    set(GV_FLAGS "${GV_FLAGS} -Weverything")
    set(GV_FLAGS "${GV_FLAGS} -Wno-variadic-macros")
    set(GV_FLAGS "${GV_FLAGS} -Wno-gnu-zero-variadic-macro-arguments")
    set(GV_FLAGS "${GV_FLAGS} -Wno-c++98-compat-pedantic")
    set(GV_FLAGS "${GV_FLAGS} -Wno-zero-length-array")

    # gtest annoyances
    set(GV_FLAGS "${GV_FLAGS} -Wno-global-constructors")
    list(APPEND CMAKE_CXX_FLAGS ${GV_FLAGS})
endif()

##############################################################################
# Build Type
# Build types are Release, Debug, Test, DebugTest
# Ex: In build folder, cmake -DCMAKE_BUILD_TYPE=Debug ..
##############################################################################
if (GV_BUILD STREQUAL "Debug" OR GV_BUILD STREQUAL "DebugTest")
    add_definitions(
            -g
            -DGV_DEBUG=20)
    message("Building Debug")
endif()

if (GV_BUILD STREQUAL "Test" OR GV_BUILD STREQUAL "DebugTest")
    message("Building Test")
    if (GV_COMPILER STREQUAL "clang++")
        add_definitions(
                -DGTEST_USE_OWN_TR1_TUPLE=1
                -DGTEST_HAS_TR1_TUPLE=1)
    endif()
endif()

##############################################################################
# Include Directories
##############################################################################
include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/src/
        ${CMAKE_CURRENT_SOURCE_DIR}/include/
        ${CMAKE_CURRENT_SOURCE_DIR}/test/)
if (GV_BUILD STREQUAL "Test" OR GV_BUILD STREQUAL "DebugTest")
    include_directories(
            ${CMAKE_CURRENT_SOURCE_DIR}/test/)
    include_directories(SYSTEM
            ${CMAKE_CURRENT_SOURCE_DIR}/gtest/
            ${CMAKE_CURRENT_SOURCE_DIR}/gtest/include/
            ${CMAKE_CURRENT_SOURCE_DIR}/gtest/lib/
            ${CMAKE_CURRENT_SOURCE_DIR}/gtest/lib/.libs/)
    list(APPEND sources-test
            ${CMAKE_CURRENT_SOURCE_DIR}/gtest/lib/.libs/libgtest_main.so
            ${CMAKE_CURRENT_SOURCE_DIR}/gtest/lib/.libs/libgtest.so)
endif()

##############################################################################
# Check for out-of-project includes
##############################################################################
include(CheckIncludeFiles)

# XXX cevans87: check_include_files macro broken in foreach?
check_include_files(zmq.hpp GV_HAVE_ZMQ)
check_include_files(dns_sd.h GV_HAVE_ZEROCONF)

set(GV_MUST_HAVE
        GV_HAVE_ZMQ
        GV_HAVE_ZEROCONF)

# FIXME We're not always using Avahi
add_definitions(
        -DGV_WITH_AVAHI=1)

#foreach(package ${GV_MUST_HAVE})
#    if (NOT ${package})
#        message(FATAL_ERROR "Failed requirement: ${package}")
#    endif()
#endforeach()

##############################################################################
# Build stuff
##############################################################################

add_library(grapevine SHARED ${sources})

if (GV_BUILD STREQUAL "Test" OR GV_BUILD STREQUAL "DebugTest")
    add_executable(test_grapevine ${sources-test})
    target_link_libraries(test_grapevine
        grapevine
        ${CMAKE_CURRENT_SOURCE_DIR}/gtest/lib/.libs/libgtest.a
        pthread
        dns_sd)
endif()
