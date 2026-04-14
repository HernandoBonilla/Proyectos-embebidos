#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include "esp_err.h"
#include "app_data.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t display_init(void);
esp_err_t display_show_data(const app_data_t *data);

// NUEVAS
void display_fill(uint16_t color);
void display_draw_pixel(int x, int y, uint16_t color);

#ifdef __cplusplus
}
#endif

#endif