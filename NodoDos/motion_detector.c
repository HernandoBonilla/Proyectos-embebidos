#include "motion_detector.h"
#include <math.h>

/**
 * @file motion_detector.c
 * @brief Implementación del detector de movimiento basado en datos del MPU6050.
 *
 * Este módulo procesa los datos del sensor inercial para:
 * - Calcular magnitudes de aceleración y rotación
 * - Estimar orientación (pitch y roll)
 * - Clasificar el estado de movimiento del usuario
 * - Detectar posibles caídas
 */

/**
 * @brief Inicializa el detector de movimiento.
 *
 * Actualmente no requiere inicialización, pero se mantiene
 * para futuras configuraciones o calibraciones.
 */
void motion_detector_init(void)
{
}

/**
 * @brief Actualiza el estado de movimiento a partir de datos del MPU6050.
 *
 * Procesa los datos del sensor para:
 * - Calcular magnitud total de aceleración
 * - Calcular magnitud total del giroscopio
 * - Estimar ángulos de orientación (pitch y roll)
 * - Determinar el estado de movimiento
 * - Detectar caídas
 *
 * @param imu Puntero a la estructura con datos del MPU6050.
 * @param out Puntero a la estructura donde se almacenan los resultados.
 */
void motion_detector_update(const mpu6050_data_t *imu, app_data_t *out)
{
    if (!imu || !out) {
        return;
    }

    /** Copia directa de datos del sensor */
    out->ax_g = imu->ax_g;
    out->ay_g = imu->ay_g;
    out->az_g = imu->az_g;

    out->gx_dps = imu->gx_dps;
    out->gy_dps = imu->gy_dps;
    out->gz_dps = imu->gz_dps;

    out->temp_c = imu->temp_c;

    /**
     * @brief Magnitud total de aceleración (norma euclidiana).
     */
    out->accel_total_g = sqrtf(
        imu->ax_g * imu->ax_g +
        imu->ay_g * imu->ay_g +
        imu->az_g * imu->az_g
    );

    /**
     * @brief Magnitud total del giroscopio (suma de valores absolutos).
     */
    out->gyro_total_dps =
        fabsf(imu->gx_dps) +
        fabsf(imu->gy_dps) +
        fabsf(imu->gz_dps);

    /**
     * @brief Cálculo del ángulo pitch (inclinación frontal).
     */
    out->pitch_deg = atan2f(
        imu->ax_g,
        sqrtf(imu->ay_g * imu->ay_g + imu->az_g * imu->az_g)
    ) * 57.2958f; /**< Conversión de radianes a grados */

    /**
     * @brief Cálculo del ángulo roll (inclinación lateral).
     */
    out->roll_deg = atan2f(
        imu->ay_g,
        sqrtf(imu->ax_g * imu->ax_g + imu->az_g * imu->az_g)
    ) * 57.2958f; /**< Conversión de radianes a grados */

    /** Inicializa bandera de caída */
    out->caida_detectada = false;

    /**
     * @brief Lógica de clasificación del estado de movimiento.
     *
     * El orden es importante: primero se evalúan eventos críticos (caída),
     * luego estados más específicos.
     */

    /** 1) Detección de caída */
    if (out->accel_total_g > 2.2f && out->gyro_total_dps > 180.0f) {
        out->estado = ESTADO_CAIDA;
        out->caida_detectada = true;
        return;
    }

    /** 2) Usuario acostado (alta inclinación) */
    if (fabsf(out->pitch_deg) > 55.0f) {
        out->estado = ESTADO_ACOSTADO;
        return;
    }

    /** 3) Caminando o movimiento moderado */
    if (out->gyro_total_dps > 70.0f) {
        out->estado = ESTADO_CAMINANDO;
        return;
    }

    /** 4) Quieto (muy poca rotación) */
    if (out->gyro_total_dps < 12.0f) {
        out->estado = ESTADO_QUIETO;
        return;
    }

    /** 5) De pie (orientación relativamente vertical) */
    if (fabsf(out->pitch_deg) < 30.0f) {
        out->estado = ESTADO_DE_PIE;
        return;
    }

    /** Estado no clasificado */
    out->estado = ESTADO_INDETERMINADO;
}

/**
 * @brief Convierte el estado de movimiento a texto.
 *
 * Útil para depuración, logs o transmisión de datos.
 *
 * @param estado Estado de movimiento.
 * @return Cadena de texto representando el estado.
 */
const char *motion_state_to_text(estado_movimiento_t estado)
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