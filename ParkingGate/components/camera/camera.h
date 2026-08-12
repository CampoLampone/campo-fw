#pragma once

#include <esp_err.h>
#include <stddef.h>
#include <stdint.h>

typedef void* camera_frame_handle_t;

esp_err_t camera_init(void);

esp_err_t camera_get_stream_frame(camera_frame_handle_t *out_handle, const uint8_t **out_buf, size_t *out_len);
esp_err_t camera_get_capture_frame(camera_frame_handle_t *out_handle, const uint8_t **out_buf, size_t *out_len, int exposure_delay_ms);
void camera_release_frame(camera_frame_handle_t handle);
