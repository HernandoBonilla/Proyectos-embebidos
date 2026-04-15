#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"

#include "app_data.h"
#include "i2c_bus.h"
#include "mpu6050.h"
#include "motion_detector.h"

/** @brief Etiqueta usada para los mensajes de log del nodo pecho */
#define TAG "NODO_PECHO"

/**
 * @brief Convierte un estado de movimiento a texto para salida CSV.
 *
 * Esta función traduce los valores de la enumeración
 * @ref estado_movimiento_t a cadenas de texto legibles,
 * útiles para registro o transmisión en formato CSV.
 *
 * @param estado Estado de movimiento a convertir.
 * @return Cadena de texto correspondiente al estado.
 */
static const char *estado_a_texto_csv(estado_movimiento_t estado)
{
    switch (estado) {
        case ESTADO_QUIETO: return "QUIETO";
        case ESTADO_DE_PIE: return "DE_PIE";
        case ESTADO_ACOSTADO: return "ACOSTADO";
        case ESTADO_CAMINANDO: return "CAMINANDO";
        case ESTADO_CAIDA: return "CAIDA";
        default: return "INDETERMINADO";
    }
}

/**
 * @brief Tarea de FreeRTOS para lectura del MPU6050 y detección de movimiento.
 *
 * Esta tarea realiza de forma periódica:
 * - Lectura de datos del sensor MPU6050
 * - Actualización de la estructura de datos de la aplicación
 * - Detección del estado de movimiento y posibles caídas
 * - Impresión de los datos en formato CSV
 *
 * El formato de salida generado es:
 * @code
 * $DATA,ax,ay,az,gx,gy,gz,temp,pitch,roll,accel_total,gyro_total,estado,caida,frame
 * @endcode
 *
 * @param pv Parámetro de la tarea, no utilizado.
 */
static void task_mpu_motion(void *pv)
{
    mpu6050_data_t imu;   /**< Estructura temporal para almacenar datos crudos/procesados del MPU6050 */
    app_data_t data = {0}; /**< Estructura de datos de aplicación inicializada en cero */
    uint32_t frame = 0;   /**< Contador de muestras adquiridas */

    while (1) {
        esp_err_t ret = mpu6050_read(&imu);

        if (ret == ESP_OK) {
            motion_detector_update(&imu, &data);
            frame++;

            printf("$DATA,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%s,%d,%lu\n",
                   data.ax_g,
                   data.ay_g,
                   data.az_g,
                   data.gx_dps,
                   data.gy_dps,
                   data.gz_dps,
                   data.temp_c,
                   data.pitch_deg,
                   data.roll_deg,
                   data.accel_total_g,
                   data.gyro_total_dps,
                   estado_a_texto_csv(data.estado),
                   data.caida_detectada ? 1 : 0,
                   (unsigned long)frame);
        } else {
            ESP_LOGE(TAG, "Error leyendo MPU6050: %s", esp_err_to_name(ret));
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/**
 * @brief Punto de entrada principal de la aplicación.
 *
 * Esta función realiza la inicialización de los módulos principales:
 * - Bus I2C
 * - Sensor MPU6050
 * - Detector de movimiento
 *
 * Posteriormente crea una tarea encargada de leer el sensor y
 * procesar el estado de movimiento de forma periódica.
 */
void app_main(void)
{
    ESP_LOGI(TAG, "Inicializando nodo pecho...");

    ESP_ERROR_CHECK(app_i2c_init());
    ESP_ERROR_CHECK(mpu6050_init(app_i2c_get_handle()));
    motion_detector_init();

    xTaskCreate(task_mpu_motion, "task_mpu_motion", 4096, NULL, 5, NULL);
}