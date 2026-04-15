#include "mpu6050.h"

#include "esp_log.h"

/**
 * @file mpu6050.c
 * @brief Implementación del driver básico para el sensor MPU6050.
 *
 * Este módulo permite:
 * - Detectar el sensor en el bus I2C
 * - Inicializarlo
 * - Leer datos crudos del acelerómetro, giroscopio y temperatura
 * - Convertir los datos a unidades físicas
 */

/** @brief Etiqueta para mensajes de log del módulo MPU6050 */
#define TAG "MPU6050"

/** @brief Dirección I2C del MPU6050 cuando AD0 = 0 */
#define MPU6050_ADDR_68      0x68

/** @brief Dirección I2C del MPU6050 cuando AD0 = 1 */
#define MPU6050_ADDR_69      0x69

/** @brief Tiempo máximo de espera para transacciones I2C, en milisegundos */
#define MPU6050_TIMEOUT_MS   100

/** @brief Frecuencia de reloj I2C usada para el dispositivo, en Hz */
#define I2C_FREQ_HZ          400000

/** @brief Registro de identificación del dispositivo */
#define REG_WHO_AM_I         0x75

/** @brief Registro de gestión de energía 1 */
#define REG_PWR_MGMT_1       0x6B

/** @brief Registro inicial para lectura secuencial de acelerómetro, temperatura y giroscopio */
#define REG_ACCEL_XOUT_H     0x3B

/** @brief Manejador estático del dispositivo MPU6050 en el bus I2C */
static i2c_master_dev_handle_t s_mpu = NULL;

/** @brief Dirección I2C detectada del MPU6050 */
static uint8_t s_mpu_addr = 0x00;

/**
 * @brief Escribe un valor en un registro del MPU6050.
 *
 * @param reg Dirección del registro.
 * @param value Valor a escribir.
 * @return esp_err_t
 * @retval ESP_OK Escritura exitosa
 * @retval != ESP_OK Error en la transmisión I2C
 */
static esp_err_t mpu_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = {reg, value};
    return i2c_master_transmit(s_mpu, buf, sizeof(buf), MPU6050_TIMEOUT_MS);
}

/**
 * @brief Lee uno o varios registros consecutivos del MPU6050.
 *
 * @param reg Dirección del primer registro a leer.
 * @param data Buffer de destino donde se almacenan los datos.
 * @param len Número de bytes a leer.
 * @return esp_err_t
 * @retval ESP_OK Lectura exitosa
 * @retval != ESP_OK Error en la transacción I2C
 */
static esp_err_t mpu_read_regs(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(s_mpu, &reg, 1, data, len, MPU6050_TIMEOUT_MS);
}

/**
 * @brief Agrega el dispositivo MPU6050 al bus I2C.
 *
 * Configura un dispositivo I2C con dirección de 7 bits y la frecuencia
 * de reloj especificada para el sensor.
 *
 * @param bus Manejador del bus I2C maestro.
 * @param addr Dirección I2C del dispositivo.
 * @param out_dev Puntero donde se almacenará el manejador del dispositivo creado.
 * @return esp_err_t
 * @retval ESP_OK Dispositivo agregado correctamente
 * @retval != ESP_OK Error al agregar el dispositivo al bus
 */
static esp_err_t mpu_add_device(i2c_master_bus_handle_t bus, uint8_t addr, i2c_master_dev_handle_t *out_dev)
{
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = I2C_FREQ_HZ,
    };

    return i2c_master_bus_add_device(bus, &dev_cfg, out_dev);
}

/**
 * @brief Inicializa el sensor MPU6050.
 *
 * La función intenta detectar el dispositivo primero en la dirección
 * I2C 0x68 y luego en 0x69. Si lo encuentra, lo despierta escribiendo
 * en el registro de gestión de energía.
 *
 * @param bus Manejador del bus I2C maestro.
 * @return esp_err_t
 * @retval ESP_OK Inicialización exitosa o ya inicializado
 * @retval ESP_ERR_INVALID_ARG El bus recibido es NULL
 * @retval ESP_ERR_NOT_FOUND No se encontró el MPU6050 en las direcciones esperadas
 * @retval != ESP_OK Otro error durante la inicialización
 */
esp_err_t mpu6050_init(i2c_master_bus_handle_t bus)
{
    if (bus == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_mpu != NULL) {
        return ESP_OK;
    }

    esp_err_t ret;
    uint8_t who = 0;
    i2c_master_dev_handle_t dev = NULL;

    /** Probar dirección I2C 0x68 */
    ret = mpu_add_device(bus, MPU6050_ADDR_68, &dev);
    if (ret == ESP_OK) {
        s_mpu = dev;
        ret = mpu_read_regs(REG_WHO_AM_I, &who, 1);
        if (ret == ESP_OK) {
            s_mpu_addr = MPU6050_ADDR_68;
            goto found;
        }
        i2c_master_bus_rm_device(dev);
        s_mpu = NULL;
    }

    /** Probar dirección I2C 0x69 */
    ret = mpu_add_device(bus, MPU6050_ADDR_69, &dev);
    if (ret == ESP_OK) {
        s_mpu = dev;
        ret = mpu_read_regs(REG_WHO_AM_I, &who, 1);
        if (ret == ESP_OK) {
            s_mpu_addr = MPU6050_ADDR_69;
            goto found;
        }
        i2c_master_bus_rm_device(dev);
        s_mpu = NULL;
    }

    ESP_LOGE(TAG, "No se encontro MPU6050 en 0x68 ni 0x69");
    return ESP_ERR_NOT_FOUND;

found:
    ESP_LOGI(TAG, "MPU6050 detectado en 0x%02X, WHO_AM_I=0x%02X", s_mpu_addr, who);

    ret = mpu_write_reg(REG_PWR_MGMT_1, 0x00);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error despertando MPU6050: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "MPU6050 inicializado correctamente");
    return ESP_OK;
}

/**
 * @brief Lee una muestra completa del MPU6050.
 *
 * Esta función obtiene 14 bytes consecutivos desde el registro
 * `REG_ACCEL_XOUT_H`, correspondientes a:
 * - Acelerómetro en X, Y, Z
 * - Temperatura
 * - Giroscopio en X, Y, Z
 *
 * Luego convierte los datos crudos a unidades físicas:
 * - Aceleración en g
 * - Velocidad angular en grados por segundo
 * - Temperatura en grados Celsius
 *
 * @param out Puntero a la estructura donde se almacenarán los datos leídos.
 * @return esp_err_t
 * @retval ESP_OK Lectura exitosa
 * @retval ESP_ERR_INVALID_STATE El dispositivo no está inicializado o el puntero es NULL
 * @retval != ESP_OK Error en la lectura I2C
 */
esp_err_t mpu6050_read(mpu6050_data_t *out)
{
    if (out == NULL || s_mpu == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t buf[14];
    esp_err_t ret = mpu_read_regs(REG_ACCEL_XOUT_H, buf, sizeof(buf));
    if (ret != ESP_OK) {
        return ret;
    }

    /** Conversión de bytes recibidos a valores crudos de aceleración */
    out->ax_raw = (int16_t)((buf[0] << 8) | buf[1]);
    out->ay_raw = (int16_t)((buf[2] << 8) | buf[3]);
    out->az_raw = (int16_t)((buf[4] << 8) | buf[5]);

    /** Conversión de bytes recibidos a temperatura cruda */
    out->temp_raw = (int16_t)((buf[6] << 8) | buf[7]);

    /** Conversión de bytes recibidos a valores crudos del giroscopio */
    out->gx_raw = (int16_t)((buf[8] << 8) | buf[9]);
    out->gy_raw = (int16_t)((buf[10] << 8) | buf[11]);
    out->gz_raw = (int16_t)((buf[12] << 8) | buf[13]);

    /** Conversión de aceleración cruda a unidades de gravedad (g) */
    out->ax_g = out->ax_raw / 16384.0f;
    out->ay_g = out->ay_raw / 16384.0f;
    out->az_g = out->az_raw / 16384.0f;

    /** Conversión de velocidad angular cruda a grados por segundo (dps) */
    out->gx_dps = out->gx_raw / 131.0f;
    out->gy_dps = out->gy_raw / 131.0f;
    out->gz_dps = out->gz_raw / 131.0f;

    /** Conversión de temperatura cruda a grados Celsius */
    out->temp_c = (out->temp_raw / 340.0f) + 36.53f;

    return ESP_OK;
}