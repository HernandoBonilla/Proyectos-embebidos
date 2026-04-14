#include "qmi8658.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define TAG "QMI8658"

#define I2C_TIMEOUT_MS                    100

#define QMI8658_ADDR_LOW                  0x6A
#define QMI8658_ADDR_HIGH                 0x6B

#define QMI8658_REG_WHO_AM_I              0x00
#define QMI8658_REG_REVISION              0x01
#define QMI8658_REG_CTRL1                 0x02
#define QMI8658_REG_CTRL2                 0x03
#define QMI8658_REG_CTRL3                 0x04
#define QMI8658_REG_CTRL5                 0x06
#define QMI8658_REG_CTRL7                 0x08
#define QMI8658_REG_TEMP_L                0x33
#define QMI8658_REG_AX_L                  0x35

#define QMI8658_CTRL2_ACCEL_2G_125HZ      0x16
#define QMI8658_CTRL3_GYRO_256DPS_125HZ   0x46
#define QMI8658_CTRL5_LPF_DEFAULT         0x00
#define QMI8658_CTRL7_ENABLE_AG           0x03

#define ACC_LSB_PER_G                     16384.0f
#define GYRO_LSB_PER_DPS                  128.0f
#define I2C_FREQ_HZ                       400000

static i2c_master_dev_handle_t qmi_dev = NULL;
static uint8_t qmi_addr = 0x00;

static esp_err_t qmi_read_reg(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(qmi_dev, &reg, 1, data, len, I2C_TIMEOUT_MS);
}

static esp_err_t qmi_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = {reg, value};
    return i2c_master_transmit(qmi_dev, buf, sizeof(buf), I2C_TIMEOUT_MS);
}

static esp_err_t qmi_add_device(i2c_master_bus_handle_t bus, uint8_t addr, i2c_master_dev_handle_t *out_dev)
{
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = I2C_FREQ_HZ,
    };

    return i2c_master_bus_add_device(bus, &dev_cfg, out_dev);
}

esp_err_t qmi8658_init(i2c_master_bus_handle_t bus)
{
    if (bus == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t who = 0;
    esp_err_t ret;
    i2c_master_dev_handle_t dev = NULL;

    ret = qmi_add_device(bus, QMI8658_ADDR_LOW, &dev);
    if (ret == ESP_OK) {
        qmi_dev = dev;
        ret = qmi_read_reg(QMI8658_REG_WHO_AM_I, &who, 1);
        if (ret == ESP_OK) {
            qmi_addr = QMI8658_ADDR_LOW;
            goto configured;
        }
        i2c_master_bus_rm_device(dev);
        qmi_dev = NULL;
    }

    ret = qmi_add_device(bus, QMI8658_ADDR_HIGH, &dev);
    if (ret == ESP_OK) {
        qmi_dev = dev;
        ret = qmi_read_reg(QMI8658_REG_WHO_AM_I, &who, 1);
        if (ret == ESP_OK) {
            qmi_addr = QMI8658_ADDR_HIGH;
            goto configured;
        }
        i2c_master_bus_rm_device(dev);
        qmi_dev = NULL;
    }

    return ESP_ERR_NOT_FOUND;

configured:
    ESP_LOGI(TAG, "Detectado en 0x%02X, WHO_AM_I=0x%02X", qmi_addr, who);

    ESP_ERROR_CHECK(qmi_write_reg(QMI8658_REG_CTRL1, 0x40));
    ESP_ERROR_CHECK(qmi_write_reg(QMI8658_REG_CTRL2, QMI8658_CTRL2_ACCEL_2G_125HZ));
    ESP_ERROR_CHECK(qmi_write_reg(QMI8658_REG_CTRL3, QMI8658_CTRL3_GYRO_256DPS_125HZ));
    ESP_ERROR_CHECK(qmi_write_reg(QMI8658_REG_CTRL5, QMI8658_CTRL5_LPF_DEFAULT));
    ESP_ERROR_CHECK(qmi_write_reg(QMI8658_REG_CTRL7, QMI8658_CTRL7_ENABLE_AG));

    vTaskDelay(pdMS_TO_TICKS(50));

    uint8_t rev = 0;
    ESP_ERROR_CHECK(qmi_read_reg(QMI8658_REG_REVISION, &rev, 1));
    ESP_LOGI(TAG, "REV=0x%02X", rev);

    return ESP_OK;
}

esp_err_t qmi8658_read(qmi8658_data_t *out)
{
    if (!out || !qmi_dev) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t buf[12];
    uint8_t temp_buf[2];

    esp_err_t ret = qmi_read_reg(QMI8658_REG_AX_L, buf, sizeof(buf));
    if (ret != ESP_OK) {
        return ret;
    }

    ret = qmi_read_reg(QMI8658_REG_TEMP_L, temp_buf, sizeof(temp_buf));
    if (ret != ESP_OK) {
        return ret;
    }

    out->ax_raw = (int16_t)((buf[1] << 8) | buf[0]);
    out->ay_raw = (int16_t)((buf[3] << 8) | buf[2]);
    out->az_raw = (int16_t)((buf[5] << 8) | buf[4]);

    out->gx_raw = (int16_t)((buf[7] << 8) | buf[6]);
    out->gy_raw = (int16_t)((buf[9] << 8) | buf[8]);
    out->gz_raw = (int16_t)((buf[11] << 8) | buf[10]);

    out->temp_raw = (int16_t)((temp_buf[1] << 8) | temp_buf[0]);

    out->ax_g = out->ax_raw / ACC_LSB_PER_G;
    out->ay_g = out->ay_raw / ACC_LSB_PER_G;
    out->az_g = out->az_raw / ACC_LSB_PER_G;

    out->gx_dps = out->gx_raw / GYRO_LSB_PER_DPS;
    out->gy_dps = out->gy_raw / GYRO_LSB_PER_DPS;
    out->gz_dps = out->gz_raw / GYRO_LSB_PER_DPS;

    out->temp_c = out->temp_raw / 256.0f;

    return ESP_OK;
}