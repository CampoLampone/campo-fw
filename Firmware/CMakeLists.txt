cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_LIST_DIR}/boards)

set(PICO_BOARD board_lampone)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(lampone)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
pico_sdk_init()

add_executable(${PROJECT_NAME} main.c tests.c)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
add_subdirectory(driver)
add_subdirectory(controller)

target_link_libraries(${PROJECT_NAME} pico_stdlib pico_stdio_usb driver_display driver_motor driver_encoder controller_pid controller_speed driver_spi ws2812)

pico_add_extra_outputs(${PROJECT_NAME})
