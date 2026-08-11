#pragma once

#include "motor.h"
#include <stdbool.h>
#include <stdint.h>

#define SUBSTEPS_PER_PULSE 64.0
#define PULSES_PER_MOTOR_ROTATION 12.0
#define GEARBOX_RATIO 51.4462

#define PULSES_PER_WHEEL_ROTATION (PULSES_PER_MOTOR_ROTATION * GEARBOX_RATIO)

#define PID_TO_PWM_SCALE 256
#define PID_I_MAX 25000.0f

#define PWM_MAX 0xFFFF

#define STALL_THRESHOLD 1.0  // in RPM
#define STALL_TIME_MS 1000    // Time in milliseconds

void compute_encoders_rpm(float delta_ms);
void speed_controller_init(float kp, float ki, float kd);
void reset_emergency_stop();
int clamp_pid_to_pwm(float val);
void clear_pid_cache();
void control_speed(int16_t target_speed[MOTOR_COUNT], float delta_ms);
float get_rpm(uint8_t side);
bool get_emergency_stop();
