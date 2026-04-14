#include "ds3231.h"

#include <stddef.h>
#include "driver/i2c_master.h"

#define DS3231_I2C_ADDR      0x68
#define DS3231_TIMEOUT_MS    100
#define I2C_FREQ_HZ          100000

#define DS3231_REG_SECONDS   0x00
#define DS3231_REG_MINUTES   0x01
#define DS3231_REG_HOURS     0x02
#define DS3231_REG_DAY       0x03
#define DS3231_REG_DATE      0x04
#define DS3231_REG_MONTH     0x05
#define DS3231_REG_YEAR      0x06

static i2c_master_dev_handle_t rtc_dev = NULL;

static uint8_t dec_to_bcd(uint8_t val)
{
    return (uint8_t)(((val / 10) << 4) | (val % 10));
}

static uint8_t bcd_to_dec(uint8_t val)
{
    return (uint8_t)(((val >> 4) * 10) + (val & 0x0F));
}

static esp_err_t ds3231_read_regs(uint8_t start_reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(rtc_dev, &start_reg, 1, data, len, DS3231_TIMEOUT_MS);
}

static esp_err_t ds3231_write_regs(uint8_t start_reg, const uint8_t *data, size_t len)
{
    uint8_t buf[16];

    if (len + 1 > sizeof(buf)) {
        return ESP_ERR_INVALID_SIZE;
    }

    buf[0] = start_reg;
    for (size_t i = 0; i < len; i++) {
        buf[i + 1] = data[i];
    }

    return i2c_master_transmit(rtc_dev, buf, len + 1, DS3231_TIMEOUT_MS);
}

esp_err_t ds3231_init(i2c_master_bus_handle_t bus)
{
    if (bus == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (rtc_dev != NULL) {
        return ESP_OK;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = DS3231_I2C_ADDR,
        .scl_speed_hz = I2C_FREQ_HZ,
    };

    return i2c_master_bus_add_device(bus, &dev_cfg, &rtc_dev);
}

esp_err_t ds3231_get_time(ds3231_time_t *out)
{
    if (out == NULL || rtc_dev == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t buf[7] = {0};
    esp_err_t ret = ds3231_read_regs(DS3231_REG_SECONDS, buf, sizeof(buf));
    if (ret != ESP_OK) {
        out->valid = false;
        return ret;
    }

    out->second = bcd_to_dec(buf[0] & 0x7F);
    out->minute = bcd_to_dec(buf[1] & 0x7F);
    out->hour   = bcd_to_dec(buf[2] & 0x3F);
    out->day    = bcd_to_dec(buf[4] & 0x3F);
    out->month  = bcd_to_dec(buf[5] & 0x1F);
    out->year   = 2000 + bcd_to_dec(buf[6]);
    out->valid  = true;

    return ESP_OK;
}

esp_err_t ds3231_set_time(const ds3231_time_t *t)
{
    if (t == NULL || rtc_dev == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t buf[7];
    buf[0] = dec_to_bcd((uint8_t)t->second);
    buf[1] = dec_to_bcd((uint8_t)t->minute);
    buf[2] = dec_to_bcd((uint8_t)t->hour);
    buf[3] = dec_to_bcd(1);
    buf[4] = dec_to_bcd((uint8_t)t->day);
    buf[5] = dec_to_bcd((uint8_t)t->month);
    buf[6] = dec_to_bcd((uint8_t)(t->year - 2000));

    return ds3231_write_regs(DS3231_REG_SECONDS, buf, sizeof(buf));
}