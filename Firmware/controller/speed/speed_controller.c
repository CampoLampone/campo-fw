#include "encoder.h"
#include "motor.h"
#include "pid.h"
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "speed_controller.h"

PID_t motors_pid[MOTOR_COUNT];
volatile bool emergencyStopFlag = false;
float stall_for_ms[MOTOR_COUNT] = {0};
float measured_rpm[MOTOR_COUNT] = {0};


typedef struct {
    float alpha;
    float y_prev;
} LowPassFilter;

void lpf_init(LowPassFilter *f, float cutoff_freq, float dt) {
    float RC = 1.0f / (2.0f * 3.14159265f * cutoff_freq);
    f->alpha = dt / (RC + dt);
    f->y_prev = 0.0f;  // initial output
}

float lpf_apply(LowPassFilter *f, float x) {
    float y = f->y_prev + f->alpha * (x - f->y_prev);
    f->y_prev = y;
    return y;
}


LowPassFilter lp_filter[MOTOR_COUNT];


void compute_encoders_rpm(float delta_ms){
    for (int side = 0; side < ENCODER_COUNT; side++) {
        substep_update(&encoders_states[side]);
        int measured_speed = encoders_states[side].speed;
        measured_rpm[side] = measured_speed / SUBSTEPS_PER_PULSE / PULSES_PER_WHEEL_ROTATION * 60.0;
    }
}

int clamp_pid_to_pwm(float val) {
    int val_int = (int)(val * PID_TO_PWM_SCALE);
    if (val_int < -PWM_MAX) return -PWM_MAX;
    if (val_int > PWM_MAX) return PWM_MAX;
    return val_int;
}

void control_motor_speed(int16_t target_speed, uint8_t side, float delta_ms){
    motors_pid[side].measured_value = lpf_apply(&lp_filter[side], measured_rpm[side]);
    motors_pid[side].dt = delta_ms / 1000.0f;
    motors_pid[side].setpoint = target_speed;
    pid_compute(&motors_pid[side]);
    motor_set_pwm(side, emergencyStopFlag ? 0 : clamp_pid_to_pwm(motors_pid[side].output));
    printf("%f,%i,", measured_rpm[side], target_speed);

    if ((fabsf(measured_rpm[side]) <= STALL_THRESHOLD) && (abs(target_speed) > 2*STALL_THRESHOLD))
        stall_for_ms[side] += delta_ms;
    else stall_for_ms[side] = 0;

    if (stall_for_ms[side] >= STALL_TIME_MS) {
        emergencyStopFlag = true;
        stall_for_ms[side] = 0;
        printf("\nMotors STALL!\n");
    }
}

void control_speed(int16_t target_speed[MOTOR_COUNT], float delta_ms) {
    for (int motor = 0; motor < MOTOR_COUNT; motor++) {
        control_motor_speed(target_speed[motor], motor, delta_ms);
    }
    printf("\n");
}

void clear_pid_cache(){
    for (int motor = 0; motor < MOTOR_COUNT; motor++) {
        motors_pid[motor].integral = 0;
        motors_pid[motor].prev_error = 0;
        motors_pid[motor].output = 0;
    }
}

void speed_controller_init(float kp, float ki, float kd) {
    for (int motor = 0; motor < MOTOR_COUNT; motor++) {
        motors_pid[motor].kp = kp;
        motors_pid[motor].ki = ki;
        motors_pid[motor].kd = kd;
        lpf_init(&lp_filter[motor], 2.0f, 0.01f);
        motors_pid[motor].integral_max = PID_I_MAX;
        motors_pid[motor].integral_min = -PID_I_MAX;
    }
}

void reset_emergency_stop() {
    emergencyStopFlag = false;
}

float get_rpm(uint8_t side){
    return measured_rpm[side];
}

bool get_emergency_stop(){
    return emergencyStopFlag;
}
