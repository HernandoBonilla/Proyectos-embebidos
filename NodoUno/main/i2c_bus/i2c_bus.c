#include "i2c_bus.h"

#include "esp_log.h"

#define TAG "I2C_BUS"

// Bus interno de la placa: QMI8658 + touch
#define I2C_INT_SDA_IO     6
#define I2C_INT_SCL_IO     7

// Bus externo por header: DS3231
#define I2C_EXT_SDA_IO     15
#define I2C_EXT_SCL_IO     16

#define I2C_PORT_INT       0
#define I2C_PORT_EXT       1
#define I2C_FREQ_HZ        400000

static i2c_master_bus_handle_t s_i2c_bus_internal = NULL;
static i2c_master_bus_handle_t s_i2c_bus_external = NULL;

esp_err_t app_i2c_bus_init(void)
{
    esp_err_t ret;

    if (s_i2c_bus_internal == NULL) {
        i2c_master_bus_config_t bus_int_cfg = {
            .i2c_port = I2C_PORT_INT,
            .sda_io_num = I2C_INT_SDA_IO,
            .scl_io_num = I2C_INT_SCL_IO,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .flags.enable_internal_pullup = true,
        };

        ret = i2c_new_master_bus(&bus_int_cfg, &s_i2c_bus_internal);
        if (ret != ESP_OK) {
            return ret;
        }

        ESP_LOGI(TAG, "Bus I2C interno inicializado en SDA=%d SCL=%d",
                 I2C_INT_SDA_IO, I2C_INT_SCL_IO);
    }

    if (s_i2c_bus_external == NULL) {
        i2c_master_bus_config_t bus_ext_cfg = {
            .i2c_port = I2C_PORT_EXT,
            .sda_io_num = I2C_EXT_SDA_IO,
            .scl_io_num = I2C_EXT_SCL_IO,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .flags.enable_internal_pullup = true,
        };

        ret = i2c_new_master_bus(&bus_ext_cfg, &s_i2c_bus_external);
        if (ret != ESP_OK) {
            return ret;
        }

        ESP_LOGI(TAG, "Bus I2C externo inicializado en SDA=%d SCL=%d",
                 I2C_EXT_SDA_IO, I2C_EXT_SCL_IO);
    }

    return ESP_OK;
}

i2c_master_bus_handle_t app_i2c_bus_get_internal_handle(void)
{
    return s_i2c_bus_internal;
}

i2c_master_bus_handle_t app_i2c_bus_get_external_handle(void)
{
    return s_i2c_bus_external;
}