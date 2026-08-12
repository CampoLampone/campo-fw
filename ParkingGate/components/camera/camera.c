#include "esp_camera.h"
#include "config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "camera.h"

struct camera_frame_settings {
    size_t width;
    size_t height;
    framesize_t frame_size;
    int quality;
};

const struct camera_frame_settings small_frame_settings = {
    .width = 96,
    .height = 96,
    .frame_size = FRAMESIZE_96X96,
    .quality = 12,
};

const struct camera_frame_settings full_frame_settings = {
    .width = 1600,
    .height = 1200,
    .frame_size = FRAMESIZE_UXGA,
    .quality = 6,
};

static const char *TAG = "Camera";
static SemaphoreHandle_t camera_mutex = NULL;

static camera_config_t camera_config = {
    .pin_pwdn  = PWDN_GPIO_NUM,
    .pin_reset = RESET_GPIO_NUM,
    .pin_xclk = XCLK_GPIO_NUM,
    .pin_sccb_sda = SIOD_GPIO_NUM,
    .pin_sccb_scl = SIOC_GPIO_NUM,

    .pin_d7 = Y9_GPIO_NUM,
    .pin_d6 = Y8_GPIO_NUM,
    .pin_d5 = Y7_GPIO_NUM,
    .pin_d4 = Y6_GPIO_NUM,
    .pin_d3 = Y5_GPIO_NUM,
    .pin_d2 = Y4_GPIO_NUM,
    .pin_d1 = Y3_GPIO_NUM,
    .pin_d0 = Y2_GPIO_NUM,
    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href = HREF_GPIO_NUM,
    .pin_pclk = PCLK_GPIO_NUM,

    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG,
    .frame_size = full_frame_settings.frame_size,
    .jpeg_quality = full_frame_settings.quality,
    .fb_count = 4,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY//CAMERA_GRAB_LATEST. Sets when buffers should be filled
};

void camera_set_frame_settings(const struct camera_frame_settings *settings) {
    sensor_t * s = esp_camera_sensor_get();
    s->set_framesize(s, settings->frame_size);
    s->set_quality(s, settings->quality);
}

void camera_drop_frames(int count) {
    for(int i = 0; i < count; i++) {
        camera_fb_t *drop_fb = esp_camera_fb_get();
        if (drop_fb) esp_camera_fb_return(drop_fb);
    }
}

esp_err_t camera_init(){
    camera_mutex = xSemaphoreCreateMutex();
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }
    camera_set_frame_settings(&small_frame_settings);
    return ESP_OK;
}



esp_err_t camera_get_stream_frame(camera_frame_handle_t *out_handle, const uint8_t **out_buf, size_t *out_len) {
    if (!out_handle || !out_buf || !out_len) return ESP_ERR_INVALID_ARG;

    while (1) {
        xSemaphoreTake(camera_mutex, portMAX_DELAY);
        camera_fb_t *fb = esp_camera_fb_get();
        xSemaphoreGive(camera_mutex);

        if (!fb) {
            ESP_LOGE(TAG, "Camera capture failed");
            return ESP_FAIL;
        }

        if (fb->format != PIXFORMAT_JPEG) {
            ESP_LOGE(TAG, "Camera format is not JPEG");
            xSemaphoreTake(camera_mutex, portMAX_DELAY);
            esp_camera_fb_return(fb);
            xSemaphoreGive(camera_mutex);
            return ESP_FAIL;
        }

        if (fb->height != small_frame_settings.height || fb->width != small_frame_settings.width) {
            xSemaphoreTake(camera_mutex, portMAX_DELAY);
            camera_set_frame_settings(&small_frame_settings);
            esp_camera_fb_return(fb);
            camera_drop_frames(FB_COUNT);
            xSemaphoreGive(camera_mutex);
            continue;
        }

        *out_handle = (camera_frame_handle_t)fb;
        *out_buf = fb->buf;
        *out_len = fb->len;
        return ESP_OK;
    }
}

esp_err_t camera_get_capture_frame(camera_frame_handle_t *out_handle, const uint8_t **out_buf, size_t *out_len, int exposure_delay_ms) {
    if (!out_handle || !out_buf || !out_len) return ESP_ERR_INVALID_ARG;

    xSemaphoreTake(camera_mutex, portMAX_DELAY);
    camera_set_frame_settings(&full_frame_settings);

    vTaskDelay(pdMS_TO_TICKS(exposure_delay_ms));

    camera_drop_frames(FB_COUNT);

    camera_fb_t *fb = esp_camera_fb_get();

    camera_set_frame_settings(&small_frame_settings);
    xSemaphoreGive(camera_mutex);

    if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        return ESP_FAIL;
    }

    *out_handle = (camera_frame_handle_t)fb;
    *out_buf = fb->buf;
    *out_len = fb->len;
    return ESP_OK;
}

void camera_release_frame(camera_frame_handle_t handle) {
    if (handle) {
        xSemaphoreTake(camera_mutex, portMAX_DELAY);
        esp_camera_fb_return((camera_fb_t *)handle);
        xSemaphoreGive(camera_mutex);
    }
}
