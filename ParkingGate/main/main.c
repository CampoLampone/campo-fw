#include "config.h"

#include "nvs_flash.h"
#include "camera.h"
#include "leds.h"
#include "gates.h"
#include "server.h"
#include "network.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main() {
    network_init();
    while(!network_is_connected()){
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    camera_init();
    leds_init();
    gates_init();
    server_start();

    while(true){
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
