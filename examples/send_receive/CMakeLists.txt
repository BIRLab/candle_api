project(send_receive)

add_executable(${PROJECT_NAME} main.c)
target_link_libraries(${PROJECT_NAME} candle_api)
