cmake_minimum_required(VERSION 3.20)
project(candle_api)

option(CANDLE_API_SHARED "Build Shared Library" OFF)
option(CANDLE_API_BUILD_EXAMPLES "Build Example Applications" OFF)
option(CANDLE_API_BUILD_PYBIND11 "Build Python Extension" OFF)
option(CANDLE_API_BUILD_NODE_JS "Build Node.js Extension" OFF)
option(CANDLE_API_TINYCTHREADS "Build with TinyCThread" OFF)

add_subdirectory(candle_api)

if (CANDLE_API_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if (CANDLE_API_BUILD_PYBIND11)
    add_subdirectory(candle_api_pybind11)
endif ()

if (CANDLE_API_BUILD_NODE_JS)
    add_subdirectory(candle_api_js)
endif ()