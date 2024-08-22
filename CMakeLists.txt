cmake_minimum_required(VERSION 3.20)
project(candle_api)

option(CANDLE_API_SHARED "Build Shared Library" OFF)
option(CANDLE_API_BUILD_EXAMPLES "Build Example Applications" OFF)
option(CANDLE_API_BUILD_PYTHON "Build Python Extension" OFF)

set(LIBUSB_BUILD_SHARED_LIBS OFF)
set(LIBUSB_BUILD_TESTING OFF)
set(LIBUSB_BUILD_EXAMPLES OFF)
set(LIBUSB_INSTALL_TARGETS OFF)
set(LIBUSB_TARGETS_INCLUDE_USING_SYSTEM ON)
set(LIBUSB_ENABLE_LOGGING OFF)
set(LIBUSB_ENABLE_DEBUG_LOGGING OFF)
add_subdirectory(libusb-cmake)

if (NOT WIN32)
    set_target_properties(usb-1.0 PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif ()

aux_source_directory(candle_api/src candle_api_srcs)

if (CANDLE_API_SHARED)
    add_library(candle_api SHARED ${candle_api_srcs})
    set_target_properties(candle_api PROPERTIES POSITION_INDEPENDENT_CODE ON)
else ()
    add_library(candle_api STATIC ${candle_api_srcs})
endif ()

target_include_directories(candle_api PUBLIC candle_api/include)
target_include_directories(candle_api PRIVATE candle_api/src)
target_link_libraries(candle_api PRIVATE usb-1.0)
set_target_properties(candle_api PROPERTIES C_STANDARD 11)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(candle_api PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(candle_api PRIVATE /experimental:c11atomics)
endif()

if (CANDLE_API_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if (CANDLE_API_BUILD_PYTHON)
    add_subdirectory(candle_api_python)
endif ()
