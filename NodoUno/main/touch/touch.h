#ifndef TOUCH_H
#define TOUCH_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool pressed;
    uint16_t x;
    uint16_t y;
} touch_data_t;

esp_err_t touch_init(i2c_master_bus_handle_t bus);
esp_err_t touch_read(touch_data_t *out);

#ifdef __cplusplus
}
#endif

#endif