#ifndef DS3231_H
#define DS3231_H

#include <stdbool.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    bool valid;
} ds3231_time_t;

esp_err_t ds3231_init(i2c_master_bus_handle_t bus);
esp_err_t ds3231_get_time(ds3231_time_t *out);
esp_err_t ds3231_set_time(const ds3231_time_t *t);

#ifdef __cplusplus
}
#endif

#endif