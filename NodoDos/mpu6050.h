#ifndef MPU6050_H
#define MPU6050_H

/**
 * @file mpu6050.h
 * @brief Interfaz del driver para el sensor MPU6050.
 *
 * Este módulo proporciona funciones para:
 * - Inicializar el sensor MPU6050
 * - Leer datos de aceleración, giroscopio y temperatura
 * - Convertir datos crudos a unidades físicas
 */

#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Estructura que almacena los datos del MPU6050.
 *
 * Incluye tanto valores crudos (RAW) como valores convertidos
 * a unidades físicas.
 */
typedef struct {

    /** @name Datos crudos del acelerómetro */
    /**@{ */
    int16_t ax_raw; /**< Aceleración cruda en eje X */
    int16_t ay_raw; /**< Aceleración cruda en eje Y */
    int16_t az_raw; /**< Aceleración cruda en eje Z */
    /**@} */

    /** @name Datos crudos del giroscopio */
    /**@{ */
    int16_t gx_raw; /**< Velocidad angular cruda en eje X */
    int16_t gy_raw; /**< Velocidad angular cruda en eje Y */
    int16_t gz_raw; /**< Velocidad angular cruda en eje Z */
    /**@} */

    /** @brief Temperatura cruda del sensor */
    int16_t temp_raw;

    /** @name Datos convertidos del acelerómetro (en g) */
    /**@{ */
    float ax_g; /**< Aceleración en eje X */
    float ay_g; /**< Aceleración en eje Y */
    float az_g; /**< Aceleración en eje Z */
    /**@} */

    /** @name Datos convertidos del giroscopio (en grados por segundo) */
    /**@{ */
    float gx_dps; /**< Velocidad angular en eje X */
    float gy_dps; /**< Velocidad angular en eje Y */
    float gz_dps; /**< Velocidad angular en eje Z */
    /**@} */

    /** @brief Temperatura del sensor en grados Celsius */
    float temp_c;

} mpu6050_data_t;

/**
 * @brief Inicializa el sensor MPU6050.
 *
 * Detecta automáticamente el sensor en las direcciones I2C 0x68 o 0x69
 * y lo configura para comenzar a operar.
 *
 * @param bus Manejador del bus I2C maestro.
 * @return esp_err_t
 * @retval ESP_OK Inicialización exitosa
 * @retval ESP_ERR_INVALID_ARG Parámetro inválido
 * @retval ESP_ERR_NOT_FOUND Sensor no detectado
 * @retval != ESP_OK Error durante la inicialización
 */
esp_err_t mpu6050_init(i2c_master_bus_handle_t bus);

/**
 * @brief Lee una muestra del sensor MPU6050.
 *
 * Obtiene datos del acelerómetro, giroscopio y temperatura,
 * tanto en formato crudo como en unidades físicas.
 *
 * @param out Puntero a la estructura donde se almacenarán los datos.
 * @return esp_err_t
 * @retval ESP_OK Lectura exitosa
 * @retval ESP_ERR_INVALID_STATE Sensor no inicializado o puntero inválido
 * @retval != ESP_OK Error en la comunicación I2C
 */
esp_err_t mpu6050_read(mpu6050_data_t *out);

#ifdef __cplusplus
}
#endif

#endif /* MPU6050_H */