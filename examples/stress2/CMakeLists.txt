project(stress2)

add_executable(${PROJECT_NAME} main.c)
target_link_libraries(${PROJECT_NAME} candle_api)
set_target_properties(${PROJECT_NAME} PROPERTIES C_STANDARD 11)
