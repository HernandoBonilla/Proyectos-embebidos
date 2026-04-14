#ifndef I2C_BUS_H
#define I2C_BUS_H

#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t app_i2c_bus_init(void);

i2c_master_bus_handle_t app_i2c_bus_get_internal_handle(void);
i2c_master_bus_handle_t app_i2c_bus_get_external_handle(void);

#ifdef __cplusplus
}
#endif

#endif