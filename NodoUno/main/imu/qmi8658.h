#ifndef QMI8658_H
#define QMI8658_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int16_t ax_raw;
    int16_t ay_raw;
    int16_t az_raw;
    int16_t gx_raw;
    int16_t gy_raw;
    int16_t gz_raw;
    int16_t temp_raw;

    float ax_g;
    float ay_g;
    float az_g;

    float gx_dps;
    float gy_dps;
    float gz_dps;

    float temp_c;
} qmi8658_data_t;

esp_err_t qmi8658_init(i2c_master_bus_handle_t bus);
esp_err_t qmi8658_read(qmi8658_data_t *out);

#ifdef __cplusplus
}
#endif

#endif