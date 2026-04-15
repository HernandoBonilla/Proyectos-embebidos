#ifndef MOTION_DETECTOR_H
#define MOTION_DETECTOR_H

/**
 * @file motion_detector.h
 * @brief Interfaz del módulo de detección de movimiento.
 *
 * Este módulo procesa los datos provenientes del sensor MPU6050
 * para estimar orientación, magnitudes de movimiento y clasificar
 * el estado del usuario, incluyendo la detección de caídas.
 */

#include "app_data.h"
#include "mpu6050.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa el detector de movimiento.
 *
 * Prepara el módulo para su uso. Actualmente no requiere
 * configuración interna, pero esta función se conserva para
 * futuras ampliaciones, como calibración o ajuste de parámetros.
 */
void motion_detector_init(void);

/**
 * @brief Procesa una muestra del MPU6050 y actualiza la salida.
 *
 * A partir de los datos del acelerómetro y giroscopio:
 * - Copia las mediciones base
 * - Calcula magnitudes totales de aceleración y giro
 * - Estima los ángulos de orientación
 * - Determina el estado de movimiento
 * - Detecta una posible caída
 *
 * @param imu Puntero a la estructura con datos del sensor MPU6050.
 * @param out Puntero a la estructura donde se almacenan los resultados.
 */
void motion_detector_update(const mpu6050_data_t *imu, app_data_t *out);

/**
 * @brief Convierte un estado de movimiento a su representación en texto.
 *
 * Esta función devuelve una cadena legible correspondiente al valor
 * de la enumeración @ref estado_movimiento_t.
 *
 * @param estado Estado de movimiento a convertir.
 * @return const char* Cadena de texto asociada al estado.
 */
const char *motion_state_to_text(estado_movimiento_t estado);

#ifdef __cplusplus
}
#endif

#endif /* MOTION_DETECTOR_H */