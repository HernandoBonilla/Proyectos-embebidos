#include "touch.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"

#define TAG "TOUCH"

// Waveshare ESP32-S3 Touch LCD 1.28
#define CST816S_ADDR            0x15
#define TP_RST_GPIO             13
#define TP_INT_GPIO             5
#define I2C_TIMEOUT_MS          100

#define REG_GESTURE_ID          0x01
#define REG_FINGER_NUM          0x02
#define REG_XPOS_H              0x03
#define REG_XPOS_L              0x04
#define REG_YPOS_H              0x05
#define REG_YPOS_L              0x06

static i2c_master_dev_handle_t s_touch = NULL;

static esp_err_t touch_read_regs(uint8_t start_reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(s_touch, &start_reg, 1, data, len, I2C_TIMEOUT_MS);
}

static void touch_hw_reset(void)
{
    gpio_config_t rst_cfg = {
        .pin_bit_mask = 1ULL << TP_RST_GPIO,
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&rst_cfg);

    gpio_set_level(TP_RST_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(TP_RST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(120));
}

static void touch_int_init(void)
{
    gpio_config_t int_cfg = {
        .pin_bit_mask = 1ULL << TP_INT_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&int_cfg);
}

esp_err_t touch_init(i2c_master_bus_handle_t bus)
{
    if (bus == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_touch != NULL) {
        return ESP_OK;
    }

    touch_hw_reset();
    touch_int_init();

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = CST816S_ADDR,
        .scl_speed_hz = 100000,   // más conservador
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &dev_cfg, &s_touch));
    ESP_LOGI(TAG, "CST816S listo en 0x%02X", CST816S_ADDR);

    return ESP_OK;
}

esp_err_t touch_read(touch_data_t *out)
{
    if (!out || !s_touch) {
        return ESP_ERR_INVALID_STATE;
    }

    memset(out, 0, sizeof(*out));

    uint8_t buf[6] = {0};
    esp_err_t ret = touch_read_regs(REG_GESTURE_ID, buf, sizeof(buf));
    if (ret != ESP_OK) {
        return ret;
    }

    uint8_t fingers = buf[1];
    if (fingers == 0) {
        out->pressed = false;
        return ESP_OK;
    }

    out->pressed = true;
    out->x = (uint16_t)(((buf[2] & 0x0F) << 8) | buf[3]);
    out->y = (uint16_t)(((buf[4] & 0x0F) << 8) | buf[5]);

    return ESP_OK;
}