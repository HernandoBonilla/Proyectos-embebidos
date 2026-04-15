#ifndef APP_DATA_H
#define APP_DATA_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Enumeración de los posibles estados de movimiento.
 */
typedef enum {
    ESTADO_QUIETO = 0,      /**< Dispositivo sin movimiento */
    ESTADO_DE_PIE,          /**< Usuario en posición vertical */
    ESTADO_ACOSTADO,        /**< Usuario acostado */
    ESTADO_CAMINANDO,       /**< Usuario en movimiento (caminando) */
    
    ESTADO_CAIDA,           /**< Se ha detectado una caída */
    ESTADO_INDETERMINADO    /**< Estado no definido o no clasificado */
} estado_movimiento_t;

/**
 * @brief Estructura que almacena los datos de sensores y estado del sistema.
 */
typedef struct {

    /** @name Datos del acelerómetro (en unidades de gravedad "g") */
    /**@{ */
    float ax_g; /**< Aceleración en eje X */
    float ay_g; /**< Aceleración en eje Y */
    float az_g; /**< Aceleración en eje Z */
    /**@} */

    /** @name Datos del giroscopio (en grados por segundo) */
    /**@{ */
    float gx_dps; /**< Velocidad angular en eje X */
    float gy_dps; /**< Velocidad angular en eje Y */
    float gz_dps; /**< Velocidad angular en eje Z */
    /**@} */

    /** @brief Temperatura del sensor en grados Celsius */
    float temp_c;

    /** @name Ángulos de orientación (en grados) */
    /**@{ */
    float pitch_deg; /**< Ángulo de inclinación (pitch) */
    float roll_deg;  /**< Ángulo de rotación lateral (roll) */
    /**@} */

    /** @brief Magnitud total de la aceleración (en g) */
    float accel_total_g;

    /** @brief Magnitud total de la velocidad angular (en dps) */
    float gyro_total_dps;

    /** @brief Estado actual del movimiento */
    estado_movimiento_t estado;

    /** @brief Indica si se ha detectado una caída */
    bool caida_detectada;

} app_data_t;

#endif