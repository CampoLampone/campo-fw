#include "server.h"
#include "camera.h"
#include "leds.h"
#include "gates.h"
#include "esp_log.h"
#include <esp_http_server.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static const char *TAG = "Server";

static void stream_task(void *pvParameters) {
    httpd_req_t *req = (httpd_req_t *)pvParameters;
    esp_err_t res = ESP_OK;
    char part_buf[64];

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(33); // ~30 fps cap

    while(res == ESP_OK){
        camera_frame_handle_t handle = NULL;
        const uint8_t *buf = NULL;
        size_t len = 0;

        if (camera_get_stream_frame(&handle, &buf, &len) != ESP_OK) {
            res = ESP_FAIL;
            break;
        }

        res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, len);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)buf, len);
        }

        camera_release_frame(handle);

        // Cap the loop at ~30 frames per second
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }

    // Cleanup async request and terminate task when client disconnects or an error occurs
    httpd_req_async_handler_complete(req);
    vTaskDelete(NULL);
}

static esp_err_t stream_handler(httpd_req_t *req){
    httpd_req_t *async_req = NULL;
    if (httpd_req_async_handler_begin(req, &async_req) != ESP_OK) {
        return ESP_FAIL;
    }
    xTaskCreate(stream_task, "stream_task", 4096, async_req, 5, NULL);
    return ESP_OK;
}

static esp_err_t capture_handler(httpd_req_t *req) {
    esp_err_t res = ESP_OK;
    int exposure_delay = 600;

    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        char* query_buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, query_buf, buf_len) == ESP_OK) {
            char param[32];
            if (httpd_query_key_value(query_buf, "delay", param, sizeof(param)) == ESP_OK) {
                exposure_delay = atoi(param);
            }
        }
        free(query_buf);
    }

    camera_frame_handle_t handle = NULL;
    const uint8_t *buf = NULL;
    size_t len = 0;

    if (camera_get_capture_frame(&handle, &buf, &len, exposure_delay) != ESP_OK) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    res = httpd_resp_set_type(req, "image/jpeg");
    if(res == ESP_OK){
        res = httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
    }
    if(res == ESP_OK){
        res = httpd_resp_send(req, (const char *)buf, len);
    }

    camera_release_frame(handle);

    return res;
}


static esp_err_t led_handler_impl(httpd_req_t *req, void (*led_func)(uint32_t)) {
    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        char* buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            char param[32];
            if (httpd_query_key_value(buf, "color", param, sizeof(param)) == ESP_OK) {
                uint32_t color = strtol(param, NULL, 16);
                led_func(color);
            }
        }
        free(buf);
    }
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}

static esp_err_t led_in_handler(httpd_req_t *req) {
    return led_handler_impl(req, led_in);
}

static esp_err_t led_out_handler(httpd_req_t *req) {
    return led_handler_impl(req, led_out);
}

static esp_err_t gate_handler_impl(httpd_req_t *req, void (*gate_func)(bool)) {
    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        char* buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            if (strcmp(buf, "open") == 0) {
                gate_func(true);
            } else if (strcmp(buf, "close") == 0) {
                gate_func(false);
            }
        }
        free(buf);
    }
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}

static esp_err_t gate_in_handler(httpd_req_t *req) {
    return gate_handler_impl(req, gate_in_set);
}

static esp_err_t gate_out_handler(httpd_req_t *req) {
    return gate_handler_impl(req, gate_out_set);
}

void server_start() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.max_open_sockets = 7;
    config.lru_purge_enable = true;

    httpd_uri_t stream_uri = {
        .uri       = "/stream",
        .method    = HTTP_GET,
        .handler   = stream_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t capture_uri = {
        .uri       = "/capture",
        .method    = HTTP_GET,
        .handler   = capture_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t led_in_uri = {
        .uri       = "/led_in",
        .method    = HTTP_GET,
        .handler   = led_in_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t led_out_uri = {
        .uri       = "/led_out",
        .method    = HTTP_GET,
        .handler   = led_out_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t gate_in_uri = {
        .uri       = "/gate_in",
        .method    = HTTP_GET,
        .handler   = gate_in_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t gate_out_uri = {
        .uri       = "/gate_out",
        .method    = HTTP_GET,
        .handler   = gate_out_handler,
        .user_ctx  = NULL
    };

    httpd_handle_t server = NULL;
    ESP_LOGI(TAG, "Starting web server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &stream_uri);
        httpd_register_uri_handler(server, &capture_uri);
        httpd_register_uri_handler(server, &led_in_uri);
        httpd_register_uri_handler(server, &led_out_uri);
        httpd_register_uri_handler(server, &gate_in_uri);
        httpd_register_uri_handler(server, &gate_out_uri);
    }
}
