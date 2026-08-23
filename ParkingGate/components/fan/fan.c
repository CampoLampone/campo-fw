#include "fan.h"
#include "config.h"
#include "driver/ledc.h"
#include "driver/temperature_sensor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "Fan";
static temperature_sensor_handle_t temp_sensor = NULL;

static void fan_control_task(void *pvParameters) {
    uint32_t current_duty = 0;
    while (1) {
        float tsens_value;
        if (temperature_sensor_get_celsius(temp_sensor, &tsens_value) == ESP_OK) {
            uint32_t target_duty = 0;
            if (tsens_value < 40.0) {
                target_duty = 0;
            } else if (tsens_value >= 65.0) {
                target_duty = 255;
            } else {
                // Linear interpolation: 40C -> 30% (76), 65C -> 100% (255)
                target_duty = 76 + ((tsens_value - 40.0) / (65.0 - 40.0)) * (255 - 76);
            }
            
            // Asymmetric smoothing: instantly speed up to handle heat spikes, 
            // but slowly coast down to prevent annoying acoustic RPM changes.
            if (target_duty > current_duty) {
                current_duty = target_duty;
            } else {
                // Decrease by at most 5 out of 255 every 2 seconds (~100s to go from 100% to 0%)
                if (current_duty > target_duty + 5) {
                    current_duty -= 5;
                } else {
                    current_duty = target_duty;
                }
            }

            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3, current_duty);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3);
            
            // Periodically log if it's hot
            if (tsens_value >= 55.0) {
                ESP_LOGI(TAG, "CPU Temp: %.1fC, Fan Duty: %d/255", tsens_value, (int)current_duty);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

esp_err_t fan_init(void) {
    // 1. Initialize Temperature Sensor
    temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(10, 80);
    esp_err_t err = temperature_sensor_install(&temp_sensor_config, &temp_sensor);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install temp sensor: %s", esp_err_to_name(err));
        return err;
    }
    temperature_sensor_enable(temp_sensor);

    // 2. Initialize LEDC for Fan (25kHz, 8-bit)
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_2,
        .duty_resolution  = LEDC_TIMER_8_BIT,
        .freq_hz          = 25000,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    err = ledc_timer_config(&ledc_timer);
    if (err != ESP_OK) return err;

    ledc_channel_config_t fan_ch = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_3,
        .timer_sel      = LEDC_TIMER_2,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = FAN_PIN,
        .duty           = 0,
        .hpoint         = 0
    };
    err = ledc_channel_config(&fan_ch);
    if (err != ESP_OK) return err;

    xTaskCreate(fan_control_task, "fan_control", 2048, NULL, 5, NULL);
    ESP_LOGI(TAG, "Fan control initialized on pin %d", FAN_PIN);
    return ESP_OK;
}
