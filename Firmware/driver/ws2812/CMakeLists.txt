add_library(ws2812 STATIC ws2812.c)

target_include_directories(ws2812 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(ws2812 PUBLIC hardware_pio)

pico_generate_pio_header(ws2812 ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)
