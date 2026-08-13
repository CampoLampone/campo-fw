#include "ws2812.pio.h"
#include "hardware/pio.h"
#include <stdbool.h>

static PIO pio;
static uint sm;

int ws2812_init(uint8_t pin){
    uint offset;
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &pio, &sm, &offset, pin, 1, true);
    ws2812_program_init(pio, sm, offset, pin, 800000, false);
    return success ? -1 : 0;
}

void ws2812_put_color(uint32_t rgb) {
    // Convert RGB to GRB because the LED expects it
    uint32_t grb = ((rgb & 0xFF0000) >> 8) | ((rgb & 0x00FF00) << 8) | (rgb & 0x0000FF);

    pio_sm_put_blocking(pio, sm, grb << 8);
}
