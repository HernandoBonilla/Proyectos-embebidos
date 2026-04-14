#ifndef PULSE_SENSOR_H
#define PULSE_SENSOR_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int raw;
    int voltage_mv;
    float filtered;
    float envelope;
    int bpm_instant;
    int bpm_avg;
    bool finger_present;
} pulse_data_t;

esp_err_t pulse_sensor_init(void);
esp_err_t pulse_sensor_update(void);
void pulse_sensor_get_data(pulse_data_t *out);

#ifdef __cplusplus
}
#endif

#endif