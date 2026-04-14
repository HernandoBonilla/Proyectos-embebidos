#include "pantalla.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "display.h"

static const char *TAG_PANTALLA = "PANTALLA";

static bool pantalla_inicializada = false;

/* ============================================================
 * CAPA BAJA REAL
 * ============================================================ */

static void lcd_hw_inicializar(void)
{
    if (display_init() == ESP_OK) {
        ESP_LOGI(TAG_PANTALLA, "LCD real inicializada");
    } else {
        ESP_LOGE(TAG_PANTALLA, "Error inicializando LCD real");
    }
}

static void lcd_hw_fill_screen(uint16_t color)
{
    display_fill(color);
}

static void lcd_hw_draw_pixel(int x, int y, uint16_t color)
{
    display_draw_pixel(x, y, color);
}

static void lcd_hw_draw_hline(int x, int y, int longitud, uint16_t color)
{
    for (int i = 0; i < longitud; i++) {
        lcd_hw_draw_pixel(x + i, y, color);
    }
}

static void lcd_hw_draw_vline(int x, int y, int longitud, uint16_t color)
{
    for (int i = 0; i < longitud; i++) {
        lcd_hw_draw_pixel(x, y + i, color);
    }
}

static void lcd_hw_fill_rect(int x, int y, int ancho, int alto, uint16_t color)
{
    for (int fila = 0; fila < alto; fila++) {
        for (int col = 0; col < ancho; col++) {
            lcd_hw_draw_pixel(x + col, y + fila, color);
        }
    }
}

/* ============================================================
 * FUENTE BITMAP MUY SIMPLE
 * ============================================================ */

static void dibujar_caracter_basico(int x, int y, char c, uint16_t color, uint16_t fondo, int escala)
{
    uint8_t patron[7] = {0};

    switch (c) {
        case '0': { uint8_t p[7] = {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}; memcpy(patron,p,7); } break;
        case '1': { uint8_t p[7] = {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}; memcpy(patron,p,7); } break;
        case '2': { uint8_t p[7] = {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}; memcpy(patron,p,7); } break;
        case '3': { uint8_t p[7] = {0x1E,0x01,0x01,0x0E,0x01,0x01,0x1E}; memcpy(patron,p,7); } break;
        case '4': { uint8_t p[7] = {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}; memcpy(patron,p,7); } break;
        case '5': { uint8_t p[7] = {0x1F,0x10,0x10,0x1E,0x01,0x01,0x1E}; memcpy(patron,p,7); } break;
        case '6': { uint8_t p[7] = {0x0E,0x10,0x10,0x1E,0x11,0x11,0x0E}; memcpy(patron,p,7); } break;
        case '7': { uint8_t p[7] = {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}; memcpy(patron,p,7); } break;
        case '8': { uint8_t p[7] = {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}; memcpy(patron,p,7); } break;
        case '9': { uint8_t p[7] = {0x0E,0x11,0x11,0x0F,0x01,0x01,0x0E}; memcpy(patron,p,7); } break;

        case 'A': { uint8_t p[7] = {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}; memcpy(patron,p,7); } break;
        case 'B': { uint8_t p[7] = {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}; memcpy(patron,p,7); } break;
        case 'C': { uint8_t p[7] = {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}; memcpy(patron,p,7); } break;
        case 'D': { uint8_t p[7] = {0x1C,0x12,0x11,0x11,0x11,0x12,0x1C}; memcpy(patron,p,7); } break;
        case 'E': { uint8_t p[7] = {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}; memcpy(patron,p,7); } break;
        case 'G': { uint8_t p[7] = {0x0E,0x11,0x10,0x17,0x11,0x11,0x0E}; memcpy(patron,p,7); } break;
        case 'I': { uint8_t p[7] = {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}; memcpy(patron,p,7); } break;
        case 'L': { uint8_t p[7] = {0x10,0x10,0x10,0x10,0x10,0x10,0x1F}; memcpy(patron,p,7); } break;
        case 'M': { uint8_t p[7] = {0x11,0x1B,0x15,0x15,0x11,0x11,0x11}; memcpy(patron,p,7); } break;
        case 'N': { uint8_t p[7] = {0x11,0x19,0x15,0x13,0x11,0x11,0x11}; memcpy(patron,p,7); } break;
        case 'O': { uint8_t p[7] = {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}; memcpy(patron,p,7); } break;
        case 'P': { uint8_t p[7] = {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}; memcpy(patron,p,7); } break;
        case 'Q': { uint8_t p[7] = {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}; memcpy(patron,p,7); } break;
        case 'R': { uint8_t p[7] = {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}; memcpy(patron,p,7); } break;
        case 'S': { uint8_t p[7] = {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}; memcpy(patron,p,7); } break;
        case 'T': { uint8_t p[7] = {0x1F,0x04,0x04,0x04,0x04,0x04,0x04}; memcpy(patron,p,7); } break;
        case 'U': { uint8_t p[7] = {0x11,0x11,0x11,0x11,0x11,0x11,0x0E}; memcpy(patron,p,7); } break;
        case 'V': { uint8_t p[7] = {0x11,0x11,0x11,0x11,0x11,0x0A,0x04}; memcpy(patron,p,7); } break;
        case 'Y': { uint8_t p[7] = {0x11,0x11,0x0A,0x04,0x04,0x04,0x04}; memcpy(patron,p,7); } break;

        case ':': { uint8_t p[7] = {0x00,0x04,0x04,0x00,0x04,0x04,0x00}; memcpy(patron,p,7); } break;
        case '.': { uint8_t p[7] = {0x00,0x00,0x00,0x00,0x00,0x06,0x06}; memcpy(patron,p,7); } break;
        case '%': { uint8_t p[7] = {0x19,0x19,0x02,0x04,0x08,0x13,0x13}; memcpy(patron,p,7); } break;
        case '-': { uint8_t p[7] = {0x00,0x00,0x00,0x1F,0x00,0x00,0x00}; memcpy(patron,p,7); } break;
        case ' ': { uint8_t p[7] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00}; memcpy(patron,p,7); } break;

        default:
            for (int i = 0; i < 7; i++) {
                patron[i] = 0x00;
            }
            break;
    }

    for (int fila = 0; fila < 7; fila++) {
        for (int col = 0; col < 5; col++) {
            bool pixel_on = (patron[fila] >> (4 - col)) & 0x01;
            uint16_t color_pixel = pixel_on ? color : fondo;

            for (int dy = 0; dy < escala; dy++) {
                for (int dx = 0; dx < escala; dx++) {
                    lcd_hw_draw_pixel(x + col * escala + dx,
                                      y + fila * escala + dy,
                                      color_pixel);
                }
            }
        }
    }
}

static void pantalla_verificar_inicializacion(void)
{
    if (!pantalla_inicializada) {
        pantalla_inicializar();
    }
}

static void pantalla_dibujar_encabezado(const char *titulo, uint16_t color)
{
    pantalla_dibujar_rectangulo(0, 0, PANTALLA_ANCHO_PIXELS, 30, COLOR_NEGRO);
    pantalla_dibujar_texto(8, 8, titulo, color, COLOR_NEGRO, 2);
    lcd_hw_draw_hline(0, 32, PANTALLA_ANCHO_PIXELS, COLOR_GRIS);
}

void pantalla_inicializar(void)
{
    lcd_hw_inicializar();
    lcd_hw_fill_screen(COLOR_NEGRO);
    pantalla_inicializada = true;
}

void pantalla_limpiar(uint16_t color_fondo)
{
    pantalla_verificar_inicializacion();
    lcd_hw_fill_screen(color_fondo);
}

void pantalla_dibujar_pixel(int x, int y, uint16_t color)
{
    pantalla_verificar_inicializacion();

    if (x < 0 || x >= PANTALLA_ANCHO_PIXELS || y < 0 || y >= PANTALLA_ALTO_PIXELS) {
        return;
    }

    lcd_hw_draw_pixel(x, y, color);
}

void pantalla_dibujar_rectangulo(int x, int y, int ancho, int alto, uint16_t color)
{
    pantalla_verificar_inicializacion();

    if (ancho <= 0 || alto <= 0) {
        return;
    }

    lcd_hw_fill_rect(x, y, ancho, alto, color);
}

void pantalla_dibujar_rectangulo_borde(int x, int y, int ancho, int alto, uint16_t color)
{
    pantalla_verificar_inicializacion();

    if (ancho <= 0 || alto <= 0) {
        return;
    }

    lcd_hw_draw_hline(x, y, ancho, color);
    lcd_hw_draw_hline(x, y + alto - 1, ancho, color);
    lcd_hw_draw_vline(x, y, alto, color);
    lcd_hw_draw_vline(x + ancho - 1, y, alto, color);
}

void pantalla_dibujar_texto(int x, int y, const char *texto, uint16_t color, uint16_t fondo, int escala)
{
    pantalla_verificar_inicializacion();

    if (texto == NULL || escala <= 0) {
        return;
    }

    int cursor_x = x;
    while (*texto != '\0') {
        dibujar_caracter_basico(cursor_x, y, *texto, color, fondo, escala);
        cursor_x += (6 * escala);
        texto++;
    }
}

void pantalla_mostrar_inicio(void)
{
    pantalla_limpiar(COLOR_NEGRO);

    pantalla_dibujar_encabezado("MONITOREO", COLOR_CYAN);
    pantalla_dibujar_texto(45, 60, "ESP32-S3", COLOR_BLANCO, COLOR_NEGRO, 2);
    pantalla_dibujar_texto(25, 100, "NODO SALUD", COLOR_VERDE, COLOR_NEGRO, 2);
    pantalla_dibujar_texto(35, 150, "LCD 240X240", COLOR_AMARILLO, COLOR_NEGRO, 2);
}

void pantalla_mostrar_ritmo(int bpm, int calidad, const char *estado_lectura)
{
    char buffer[64];

    pantalla_limpiar(COLOR_NEGRO);
    pantalla_dibujar_encabezado("RITMO", COLOR_ROJO);

    pantalla_dibujar_texto(90, 45, "BPM", COLOR_BLANCO, COLOR_NEGRO, 2);

    snprintf(buffer, sizeof(buffer), "%d", bpm);
    pantalla_dibujar_texto(80, 80, buffer, COLOR_AMARILLO, COLOR_NEGRO, 3);

    snprintf(buffer, sizeof(buffer), "CAL:%d", calidad);
    pantalla_dibujar_texto(50, 145, buffer, COLOR_VERDE, COLOR_NEGRO, 2);

    pantalla_dibujar_texto(20, 185, "ESTADO:", COLOR_BLANCO, COLOR_NEGRO, 1);
    pantalla_dibujar_texto(20, 200, estado_lectura ? estado_lectura : "N/A", COLOR_CYAN, COLOR_NEGRO, 1);
}

void pantalla_mostrar_spo2(float spo2, const char *estado_lectura)
{
    char buffer[64];

    pantalla_limpiar(COLOR_NEGRO);
    pantalla_dibujar_encabezado("SPO2", COLOR_AZUL);

    pantalla_dibujar_texto(85, 45, "SPO2", COLOR_BLANCO, COLOR_NEGRO, 2);

    snprintf(buffer, sizeof(buffer), "%.1f%%", spo2);
    pantalla_dibujar_texto(55, 80, buffer, COLOR_AMARILLO, COLOR_NEGRO, 3);

    pantalla_dibujar_texto(20, 185, "ESTADO:", COLOR_BLANCO, COLOR_NEGRO, 1);
    pantalla_dibujar_texto(20, 200, estado_lectura ? estado_lectura : "N/A", COLOR_CYAN, COLOR_NEGRO, 1);
}

void pantalla_mostrar_movimiento(float aceleracion_promedio_g,
                                 float giro_promedio_dps,
                                 estado_movimiento_t estado_movimiento)
{
    char buffer[64];
    const char *texto = pantalla_estado_movimiento_a_texto(estado_movimiento);
    uint16_t color_estado = COLOR_AMARILLO;

    pantalla_limpiar(COLOR_NEGRO);
    pantalla_dibujar_encabezado("MOVIMIENTO", COLOR_VERDE);

    switch (estado_movimiento) {
        case MOVIMIENTO_QUIETO:
            color_estado = COLOR_CYAN;
            break;
        case MOVIMIENTO_CAMINANDO:
            color_estado = COLOR_VERDE;
            break;
        case MOVIMIENTO_CAIDA:
            color_estado = COLOR_ROJO;
            break;
        case MOVIMIENTO_INDETERMINADO:
        default:
            color_estado = COLOR_AMARILLO;
            break;
    }

    pantalla_dibujar_texto(35, 55, texto, color_estado, COLOR_NEGRO, 2);

    snprintf(buffer, sizeof(buffer), "A:%.2fG", aceleracion_promedio_g);
    pantalla_dibujar_texto(35, 120, buffer, COLOR_BLANCO, COLOR_NEGRO, 2);

    snprintf(buffer, sizeof(buffer), "G:%.2fDPS", giro_promedio_dps);
    pantalla_dibujar_texto(20, 155, buffer, COLOR_BLANCO, COLOR_NEGRO, 2);
}

void pantalla_mostrar_alerta(const char *mensaje, uint16_t color_fondo)
{
    pantalla_limpiar(color_fondo);
    pantalla_dibujar_rectangulo_borde(10, 10, 220, 220, COLOR_BLANCO);
    pantalla_dibujar_texto(55, 50, "ALERTA", COLOR_BLANCO, color_fondo, 3);
    pantalla_dibujar_texto(20, 120, mensaje ? mensaje : "SIN MENSAJE", COLOR_BLANCO, color_fondo, 2);
}

const char *pantalla_estado_movimiento_a_texto(estado_movimiento_t estado)
{
    switch (estado) {
        case MOVIMIENTO_QUIETO:
            return "QUIETO";
        case MOVIMIENTO_CAMINANDO:
            return "CAMINANDO";
        case MOVIMIENTO_CAIDA:
            return "CAIDA";
        case MOVIMIENTO_INDETERMINADO:
        default:
            return "INDETERMINADO";
    }
}