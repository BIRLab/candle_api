cmake_minimum_required(VERSION 3.20)
project(candle_api_pybind11 CXX)

add_subdirectory(pybind11)

pybind11_add_module(candle_api_pybind11 src/bindings.cpp)
target_link_libraries(candle_api_pybind11 PRIVATE candle_api)
set_target_properties(candle_api_pybind11 PROPERTIES OUTPUT_NAME bindings CXX_STANDARD 17)
