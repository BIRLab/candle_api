cmake_minimum_required(VERSION 3.20)
project(candle_api_library)

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

aux_source_directory(src candle_api_srcs)

if (CANDLE_API_SHARED)
    add_library(candle_api SHARED ${candle_api_srcs})
    set_target_properties(candle_api PROPERTIES POSITION_INDEPENDENT_CODE ON)
else ()
    add_library(candle_api STATIC ${candle_api_srcs})
endif ()

target_include_directories(candle_api PUBLIC include)
target_include_directories(candle_api PRIVATE src)
target_link_libraries(candle_api PRIVATE usb-1.0)

if (CANDLE_API_TINYCTHREADS)
    target_sources(candle_api PRIVATE tinycthread/tinycthread.c)
    target_include_directories(candle_api PRIVATE tinycthread)
    set_target_properties(candle_api PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_compile_definitions(candle_api PRIVATE USING_TINYCTHREADS)
else ()
    set_target_properties(candle_api PROPERTIES C_STANDARD 11)
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(candle_api PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(candle_api PRIVATE /experimental:c11atomics)
endif()
