cmake_minimum_required(VERSION 2.8)
project (grapevine)

# FIXME cevans87: Detect and build gtest locally as well using same compiler as
# this project. For clang++, ./configure CC=clang CXX=clang++ CFLAGS="-02 -g"
# CXXFLAGS="-02 -g -stdlib=libc++"

# FIXME cevans87: stop forcing clang++, just prefer it.
set(GV_COMPILER "clang++")
#if (NOT GV_COMPILER)
#    if(CMAKE_SYSTEM_NAME MATCHES "Linux")
#        set(GV_COMPILER "g++")
#    elseif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
#        set(GV_COMPILER "clang++")
#    endif()
#endif()

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
        gv_zeroconf.cpp
        )

set(c-sources-test
        gv_test.cpp
        test_gv_zeroconf.cpp
        test_gv_channel.cpp
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
set(GV_FLAGS "${GV_FLAGS} --std=c++11")
if (GV_COMPILER STREQUAL "clang++")
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
    set(GV_FLAGS "${GV_FLAGS} -Wno-padded")

    # gtest annoyances
    set(GV_FLAGS "${GV_FLAGS} -Wno-global-constructors")
endif()

list(APPEND CMAKE_CXX_FLAGS ${GV_FLAGS})

##############################################################################
# Build Type
# Build types are release, test
# Ex: In build folder, cmake -DCMAKE_BUILD_TYPE=Debug ..
##############################################################################

# FIXME find a better way to set debug levels than this. Keeping this synced
# with the implemented levels is a maintenance problem.
if (GV_DEBUG_LEVEL)
    add_definitions(-g)
    if (GV_DEBUG_LEVEL STREQUAL "OFF")
        add_definitions(-DGV_DEBUG_LEVEL=0)
    elseif (GV_DEBUG_LEVEL STREQUAL "SEVERE")
        add_definitions(-DGV_DEBUG_LEVEL=1)
    elseif (GV_DEBUG_LEVEL STREQUAL "ERROR")
        add_definitions(-DGV_DEBUG_LEVEL=2)
    elseif (GV_DEBUG_LEVEL STREQUAL "WARNING")
        add_definitions(-DGV_DEBUG_LEVEL=3)
    elseif (GV_DEBUG_LEVEL STREQUAL "INFO")
        add_definitions(-DGV_DEBUG_LEVEL=4)
    elseif (GV_DEBUG_LEVEL STREQUAL "ENTRY")
        add_definitions(-DGV_DEBUG_LEVEL=5)
    elseif (GV_DEBUG_LEVEL STREQUAL "PARM")
        add_definitions(-DGV_DEBUG_LEVEL=6)
    elseif (GV_DEBUG_LEVEL STREQUAL "DEBUG")
        add_definitions(-DGV_DEBUG_LEVEL=7)
    elseif (GV_DEBUG_LEVEL STREQUAL "EXPECTED")
        add_definitions(-DGV_DEBUG_LEVEL=8)
    else()
        message(FATAL_ERROR "Invalid debug level: ${GV_DEBUG_LEVEL}")
    endif()
    message("Building with GV_DEBUG_LEVEL=${GV_DEBUG_LEVEL}")
endif()

if (GV_BUILD STREQUAL "test")
    message("Building test")
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
if (GV_BUILD STREQUAL "test")
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

if (GV_BUILD STREQUAL "test")
    add_executable(test_grapevine ${sources-test})
    target_link_libraries(test_grapevine
        grapevine
        ${CMAKE_CURRENT_SOURCE_DIR}/gtest/lib/.libs/libgtest.a
        pthread
        dns_sd)
endif()
