#include "leds.h"
#include "config.h"
#include "led_strip.h"
#include "esp_log.h"

static const char *TAG = "LEDs";
static led_strip_handle_t led_strip;

esp_err_t leds_init(void) {
    led_strip_config_t strip_config = {
        .strip_gpio_num = NEOPIXEL_PIN,
        .max_leds = 2,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags.invert_out = false,
    };

    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };

    esp_err_t err = led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NeoPixel strip");
        return err;
    }

    led_strip_clear(led_strip);
    ESP_LOGI(TAG, "NeoPixels initialized on pin %d", NEOPIXEL_PIN);
    return ESP_OK;
}

static void set_led_color(uint32_t index, uint32_t hex_color) {
    if (!led_strip) return;
    
    uint8_t r = (hex_color >> 16) & 0xFF;
    uint8_t g = (hex_color >> 8) & 0xFF;
    uint8_t b = hex_color & 0xFF;

    led_strip_set_pixel(led_strip, index, r, g, b);
    led_strip_refresh(led_strip);
}

void led_in(uint32_t hex_color) {
    set_led_color(0, hex_color);
}

void led_out(uint32_t hex_color) {
    set_led_color(1, hex_color);
}
