#pragma once
#include <stdint.h>
#include <esp_err.h>

esp_err_t leds_init(void);
void led_in(uint32_t hex_color);
void led_out(uint32_t hex_color);
