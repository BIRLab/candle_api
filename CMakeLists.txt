cmake_minimum_required(VERSION 3.20)
project(candle_api)

option(CANDLE_API_BUILD_EXAMPLES "Build Example Applications" OFF)

set(LIBUSB_BUILD_SHARED_LIBS OFF)
set(LIBUSB_BUILD_TESTING OFF)
set(LIBUSB_BUILD_EXAMPLES OFF)
set(LIBUSB_INSTALL_TARGETS OFF)
set(LIBUSB_TARGETS_INCLUDE_USING_SYSTEM ON)
set(LIBUSB_ENABLE_LOGGING OFF)
set(LIBUSB_ENABLE_DEBUG_LOGGING OFF)
add_subdirectory(libusb-cmake)

aux_source_directory(candle_api/src candle_api_srcs)
add_library(candle_api STATIC ${candle_api_srcs})
target_include_directories(candle_api PUBLIC candle_api/include)
target_include_directories(candle_api PRIVATE candle_api/src)
target_link_libraries(candle_api PRIVATE usb-1.0)
set_target_properties(candle_api PROPERTIES POSITION_INDEPENDENT_CODE ON C_STANDARD 11)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(candle_api PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(CANDLE_API_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
