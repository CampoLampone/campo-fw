#include "speed_controller.h"
#include <stdint.h>
#include <stdio.h>
#include "pico/stdio_usb.h"
#include <pico/time.h>
#include <string.h>
#include "hardware/i2c.h"
#include "motor.h"
#include "encoder.h"
#include "display.h"
#include "config.h"
#include "spi.h"
#include "spi_cmds.h"
#include "ws2812.h"

#if TEST_MODE > 0
#include "tests.h"
#endif

enum command {
    COMMAND_SPEED,
    COMMAND_POSITION,
    COMMAND_BRAKE,
    COMMAND_STOP,
    COMMAND_COAST
};

int16_t speed_target[MOTOR_COUNT] = {0, 0};
int8_t position_target[MOTOR_COUNT] = {0, 0};
uint8_t current_cmd = COMMAND_COAST;
substep_state_t encoders_states[ENCODER_COUNT];
display_data_t display_struct;

void spi_callback(uint8_t *data){
    switch (data[0]) {
        case SPI_CMD_SET_SPEED_BASE:
            current_cmd = COMMAND_SPEED;
            speed_target[0] = (data[1] << 8) | data[2];
            break;
        case SPI_CMD_SET_SPEED_BASE + SPI_CMD_NEXT_MOTOR:
            current_cmd = COMMAND_SPEED;
            speed_target[1] = (data[1] << 8) | data[2];
            break;
        case SPI_CMD_SET_POSITION_BASE:
            current_cmd = COMMAND_POSITION;
            position_target[0] = data[1];
            speed_target[0] = (data[2] << 8) | data[3];
            break;
        case SPI_CMD_SET_POSITION_BASE + SPI_CMD_NEXT_MOTOR:
            current_cmd = COMMAND_POSITION;
            position_target[1] = data[1];
            speed_target[1] = (data[2] << 8) | data[3];
            break;
        case SPI_CMD_COAST:
            current_cmd = COMMAND_COAST;
            break;
        case SPI_CMD_BRAKE:
            current_cmd = COMMAND_BRAKE;
            break;
        case SPI_CMD_REL_ESTOP:
            reset_emergency_stop();
            clear_pid_cache();
            break;
        case SPI_CMD_SET_HOSTNAME:
            strcpy(display_struct.ip, (char*)&data[1]);
            break;
        case SPI_CMD_SET_PID: // Better this than doing magic with floats on the other side
            {
                float kp = (uint16_t)((data[1] << 8) | data[2]) / 1000.0;
                float ki = (uint16_t)((data[3] << 8) | data[4]) / 1000.0;
                float kd = (uint16_t)((data[5] << 8) | data[6]) / 1000.0;
                // Seems like nothing wrong should happen when you call init again
                speed_controller_init(kp, ki, kd);
            }
            break;
        case SPI_CMD_SET_RGB_VALUE:
            {
                uint32_t rgb = (data[1] << 16) | (data[2] << 8) | data[3];
                ws2812_put_color(rgb);
            }
            break;

    }
}


int main() {
    stdio_usb_init();
    sleep_ms(2000);
    printf("Program Started\n");
#if TEST_MODE == 0
    motor_init();
    encoders_init();
    spi_init(spi_callback);
    speed_controller_init(PID_KP, PID_KI, PID_KD);
    reset_emergency_stop();
    i2c_init(i2c_default, 400 * 1000);
    ssd1306_gpio_init(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN);
    ws2812_init(1);
    ws2812_put_color(0x000000); // Set it black

    ssd1306_t disp;
    disp.external_vcc=false;

    // Check if the device is present

    int ret = i2c_read_timeout_us(i2c_default, 0x3C, NULL, 1, false, 10000);

    if (ret >= 0) {
        printf("I2C device found at address 0x%02X\n", 0x3C);
        ssd1306_init(&disp, 128, 64, true, 0x3C, i2c_default);
        ssd1306_clear(&disp);
    } else {
        printf("No I2C device found at address 0x%02X. Is the display connected correctly?\n", 0x3C);
    }

    absolute_time_t last_time = get_absolute_time();
    int wrum_time = last_time;
    while (true) {
        absolute_time_t current_time = get_absolute_time();
        int64_t delta_us = absolute_time_diff_us(last_time, current_time);
        last_time = current_time;

        compute_encoders_rpm(delta_us / 1000.0);
        switch (current_cmd) {
            case (COMMAND_SPEED):
                control_speed(speed_target, delta_us / 1000.0);
                break;
            case (COMMAND_POSITION):
                // TODO: Implement position control
                // control_position(position_target, speed_target, delta_us / 1000.0);
                break;
            case (COMMAND_COAST):
                for (int motor = 0; motor < MOTOR_COUNT; motor++) {
                    motor_coast(motor);
                }
                break;
            case (COMMAND_BRAKE):
                for (int motor = 0; motor < MOTOR_COUNT; motor++) {
                    motor_brake(motor);
                }
                break;
        }

        if (ret >= 0) {
            snprintf(display_struct.msg, DISP_BUF(MSG_SCALE), get_emergency_stop() ? "ESTOP" : "     ");
            snprintf(display_struct.stuff, DISP_BUF(STUFF_SCALE), "%4.0f  %4.0f", get_rpm(MOTOR_LEFT), get_rpm(MOTOR_RIGHT));
            ssd1306_draw_struct(&disp, &display_struct);
            ssd1306_show(&disp);
        }

        sleep_ms(10); // loop delay for PID
    }
#elif TEST_MODE > 0
    do_tests();
#endif

}
