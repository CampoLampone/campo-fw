#pragma once
typedef struct {
    float kp;
    float ki;
    float kd;

    float integral_min;
    float integral_max;

    float prev_error;
    float integral;
    float output;

    float setpoint;
    float measured_value;
    float dt;
} PID_t;

void pid_compute(PID_t *pid);
