
#include "includes/pid.h"

void pid_compute(PID_t *pid) {
    if (pid->dt <= 0.0f) return;

    float error = pid->setpoint - pid->measured_value;

    pid->integral += error * pid->dt;
    if (pid->integral > pid->integral_max) pid->integral = pid->integral_max;
    if (pid->integral < pid->integral_min) pid->integral = pid->integral_min;

    float derivative = (error - pid->prev_error) / pid->dt;

    pid->output = pid->kp * error + pid->ki * pid->integral + pid->kd * derivative;
    pid->prev_error = error;
}
