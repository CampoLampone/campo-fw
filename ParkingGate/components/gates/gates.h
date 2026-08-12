#pragma once
#include <stdbool.h>
#include <esp_err.h>

esp_err_t gates_init(void);
void gate_in_set(bool open);
void gate_out_set(bool open);
