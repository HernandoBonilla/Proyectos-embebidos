#ifndef COMMON_PACKET_H
#define COMMON_PACKET_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    ESTADO_QUIETO = 0,
    ESTADO_DE_PIE,
    ESTADO_ACOSTADO,
    ESTADO_CAMINANDO,
    ESTADO_CAIDA,
    ESTADO_INDETERMINADO
} estado_movimiento_t;

typedef struct {
    uint8_t  node_id;          // 2 = nodo pecho
    uint32_t frame;
    float ax_g;
    float ay_g;
    float az_g;
    float gx_dps;
    float gy_dps;
    float gz_dps;
    float temp_c;
    float pitch_deg;
    float roll_deg;
    float accel_total_g;
    float gyro_total_dps;
    uint8_t estado;
    uint8_t caida;
} paquete_pecho_t;

#endif