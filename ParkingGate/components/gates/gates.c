#include "gates.h"
#include "config.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "Gates";

// 50Hz = 20ms period.
// With 14-bit resolution, 20ms = 16384 ticks.
// 1.0ms pulse (0 deg) = 1.0 / 20.0 * 16384 = 819
// 2.0ms pulse (90 deg) = 2.0 / 20.0 * 16384 = 1638 (adjusting for cheap SG90s where 1.0ms delta = 90deg)

#define SERVO_MIN_PULSEWIDTH 819
#define SERVO_MAX_PULSEWIDTH 1638

esp_err_t gates_init(void) {
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_1,
        .duty_resolution  = LEDC_TIMER_14_BIT,
        .freq_hz          = 50,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    esp_err_t err = ledc_timer_config(&ledc_timer);
    if (err != ESP_OK) return err;

    ledc_channel_config_t in_ch = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_1,
        .timer_sel      = LEDC_TIMER_1,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = SERVO_IN_PIN,
        .duty           = SERVO_MIN_PULSEWIDTH,
        .hpoint         = 0
    };
    err = ledc_channel_config(&in_ch);
    if (err != ESP_OK) return err;

    ledc_channel_config_t out_ch = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_2,
        .timer_sel      = LEDC_TIMER_1,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = SERVO_OUT_PIN,
        .duty           = SERVO_MIN_PULSEWIDTH,
        .hpoint         = 0
    };
    err = ledc_channel_config(&out_ch);
    if (err != ESP_OK) return err;

    // Enable hardware fading for smooth servo movement
    ledc_fade_func_install(0);

    ESP_LOGI(TAG, "Gate servos initialized on pins %d and %d", SERVO_IN_PIN, SERVO_OUT_PIN);
    return ESP_OK;
}

void gate_in_set(bool open) {
    uint32_t duty = open ? SERVO_MAX_PULSEWIDTH : SERVO_MIN_PULSEWIDTH;
    ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, duty, 1500); // 1.5 seconds smooth sweep
    ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, LEDC_FADE_NO_WAIT);
    ESP_LOGI(TAG, "Gate IN set to %s (smooth)", open ? "OPEN" : "CLOSE");
}

void gate_out_set(bool open) {
    uint32_t duty = open ? SERVO_MAX_PULSEWIDTH : SERVO_MIN_PULSEWIDTH;
    ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, duty, 1500); // 1.5 seconds smooth sweep
    ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, LEDC_FADE_NO_WAIT);
    ESP_LOGI(TAG, "Gate OUT set to %s (smooth)", open ? "OPEN" : "CLOSE");
}
