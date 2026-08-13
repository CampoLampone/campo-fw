#pragma once
#include <stdint.h>

int ws2812_init(uint8_t pin);
void ws2812_put_color(uint32_t rgb);
