#ifndef PANTALLA_H
#define PANTALLA_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PANTALLA_ANCHO_PIXELS   240
#define PANTALLA_ALTO_PIXELS    240

/* Colores RGB565 */
#define COLOR_NEGRO             0x0000
#define COLOR_BLANCO            0xFFFF
#define COLOR_ROJO              0xF800
#define COLOR_VERDE             0x07E0
#define COLOR_AZUL              0x001F
#define COLOR_AMARILLO          0xFFE0
#define COLOR_CYAN              0x07FF
#define COLOR_MAGENTA           0xF81F
#define COLOR_GRIS              0x8410
#define COLOR_NARANJA           0xFD20

typedef enum {
    MOVIMIENTO_QUIETO = 0,
    MOVIMIENTO_CAMINANDO,
    MOVIMIENTO_CAIDA,
    MOVIMIENTO_INDETERMINADO
} estado_movimiento_t;

/* API pública */
void pantalla_inicializar(void);
void pantalla_limpiar(uint16_t color_fondo);

void pantalla_dibujar_pixel(int x, int y, uint16_t color);
void pantalla_dibujar_rectangulo(int x, int y, int ancho, int alto, uint16_t color);
void pantalla_dibujar_rectangulo_borde(int x, int y, int ancho, int alto, uint16_t color);
void pantalla_dibujar_texto(int x, int y, const char *texto, uint16_t color, uint16_t fondo, int escala);

void pantalla_mostrar_inicio(void);
void pantalla_mostrar_ritmo(int bpm, int calidad, const char *estado_lectura);
void pantalla_mostrar_spo2(float spo2, const char *estado_lectura);
void pantalla_mostrar_movimiento(float aceleracion_promedio_g,
                                 float giro_promedio_dps,
                                 estado_movimiento_t estado_movimiento);
void pantalla_mostrar_alerta(const char *mensaje, uint16_t color_fondo);

const char *pantalla_estado_movimiento_a_texto(estado_movimiento_t estado);

#ifdef __cplusplus
}
#endif

#endif