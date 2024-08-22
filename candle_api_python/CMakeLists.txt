cmake_minimum_required(VERSION 3.20)
project(candle_api_python)

find_package(Python COMPONENTS Development.Module)
Python_add_library(candle_api_python MODULE candle_api.c)
set_target_properties(candle_api_python PROPERTIES OUTPUT_NAME candle_api)
target_link_libraries(candle_api_python PRIVATE candle_api)
