cmake_minimum_required(VERSION 3.20)
project(candle_api)

option(CANDLE_API_SHARED "Build Shared Library" OFF)
option(CANDLE_API_BUILD_EXAMPLES "Build Example Applications" OFF)
option(CANDLE_API_BUILD_PYTHON "Build Python Extension" OFF)

add_subdirectory(candle_api)

if (CANDLE_API_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if (CANDLE_API_BUILD_PYTHON)
    add_subdirectory(candle_api_python)
endif ()
