#include "display.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_heap_caps.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"

#define TAG "DISPLAY"

// ==========================
// LCD GC9A01 MANUAL
// ==========================
#define LCD_HOST                 SPI2_HOST
#define LCD_H_RES                240
#define LCD_V_RES                240
#define LCD_PIXEL_CLOCK_HZ       (40 * 1000 * 1000)

#define PIN_NUM_LCD_SCLK         10
#define PIN_NUM_LCD_MOSI         11
#define PIN_NUM_LCD_MISO         (-1)
#define PIN_NUM_LCD_CS           9
#define PIN_NUM_LCD_DC           8
#define PIN_NUM_LCD_RST          14
#define PIN_NUM_LCD_BK_LIGHT     2




static esp_lcd_panel_io_handle_t lcd_io = NULL;
static bool s_display_ready = false;

// ==========================
// COLORES RGB565
// ==========================
#define RGB565(r, g, b)  (uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))

#define COLOR_BLACK   RGB565(0,   0,   0)
#define COLOR_WHITE   RGB565(255, 255, 255)
#define COLOR_GREEN   RGB565(0,   255, 0)
#define COLOR_CYAN    RGB565(0,   255, 255)
#define COLOR_YELLOW  RGB565(255, 255, 0)
#define COLOR_GRAY    RGB565(90,  90,  90)

// ==========================
// FUENTE 5x7 SIMPLE
// ==========================
static const uint8_t FONT_DIGITS[10][5] = {
    {0x3E, 0x51, 0x49, 0x45, 0x3E},
    {0x00, 0x42, 0x7F, 0x40, 0x00},
    {0x42, 0x61, 0x51, 0x49, 0x46},
    {0x21, 0x41, 0x45, 0x4B, 0x31},
    {0x18, 0x14, 0x12, 0x7F, 0x10},
    {0x27, 0x45, 0x45, 0x45, 0x39},
    {0x3C, 0x4A, 0x49, 0x49, 0x30},
    {0x01, 0x71, 0x09, 0x05, 0x03},
    {0x36, 0x49, 0x49, 0x49, 0x36},
    {0x06, 0x49, 0x49, 0x29, 0x1E}
};

static const uint8_t FONT_A[5] = {0x7E, 0x11, 0x11, 0x11, 0x7E};
static const uint8_t FONT_B[5] = {0x7F, 0x49, 0x49, 0x49, 0x36};
static const uint8_t FONT_D[5] = {0x7F, 0x41, 0x41, 0x22, 0x1C};
static const uint8_t FONT_E[5] = {0x7F, 0x49, 0x49, 0x49, 0x41};
static const uint8_t FONT_F[5] = {0x7F, 0x09, 0x09, 0x09, 0x01};
static const uint8_t FONT_G[5] = {0x3E, 0x41, 0x49, 0x49, 0x7A};
static const uint8_t FONT_H[5] = {0x7F, 0x08, 0x08, 0x08, 0x7F};
static const uint8_t FONT_I[5] = {0x00, 0x41, 0x7F, 0x41, 0x00};
static const uint8_t FONT_L[5] = {0x7F, 0x40, 0x40, 0x40, 0x40};
static const uint8_t FONT_M[5] = {0x7F, 0x02, 0x04, 0x02, 0x7F};
static const uint8_t FONT_N[5] = {0x7F, 0x04, 0x08, 0x10, 0x7F};
static const uint8_t FONT_O[5] = {0x3E, 0x41, 0x41, 0x41, 0x3E};
static const uint8_t FONT_P[5] = {0x7F, 0x09, 0x09, 0x09, 0x06};
static const uint8_t FONT_R[5] = {0x7F, 0x09, 0x19, 0x29, 0x46};
static const uint8_t FONT_S[5] = {0x46, 0x49, 0x49, 0x49, 0x31};
static const uint8_t FONT_T[5] = {0x01, 0x01, 0x7F, 0x01, 0x01};
static const uint8_t FONT_U[5] = {0x3F, 0x40, 0x40, 0x40, 0x3F};
static const uint8_t FONT_Y[5] = {0x07, 0x08, 0x70, 0x08, 0x07};

static const uint8_t FONT_COLON[5] = {0x00, 0x36, 0x36, 0x00, 0x00};
static const uint8_t FONT_DASH[5]  = {0x08, 0x08, 0x08, 0x08, 0x08};
static const uint8_t FONT_DOT[5]   = {0x00, 0x60, 0x60, 0x00, 0x00};
static const uint8_t FONT_SPACE[5] = {0x00, 0x00, 0x00, 0x00, 0x00};

static inline uint16_t color_to_be(uint16_t c)
{
    return (uint16_t)((c << 8) | (c >> 8));
}

static void lcd_cmd(uint8_t cmd)
{
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(lcd_io, cmd, NULL, 0));
}

static void lcd_data(uint8_t cmd, const void *data, size_t len)
{
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(lcd_io, cmd, data, len));
}

static void lcd_backlight_on(void)
{
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << PIN_NUM_LCD_BK_LIGHT,
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    ESP_ERROR_CHECK(gpio_set_level(PIN_NUM_LCD_BK_LIGHT, 1));
}

static void lcd_reset_hw(void)
{
    gpio_config_t rst_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << PIN_NUM_LCD_RST,
    };
    ESP_ERROR_CHECK(gpio_config(&rst_gpio_config));

    gpio_set_level(PIN_NUM_LCD_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(PIN_NUM_LCD_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(PIN_NUM_LCD_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(120));
}

static void lcd_set_window(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye)
{
    uint8_t data[4];

    data[0] = (xs >> 8) & 0xFF;
    data[1] = xs & 0xFF;
    data[2] = (xe >> 8) & 0xFF;
    data[3] = xe & 0xFF;
    lcd_data(0x2A, data, 4);

    data[0] = (ys >> 8) & 0xFF;
    data[1] = ys & 0xFF;
    data[2] = (ye >> 8) & 0xFF;
    data[3] = ye & 0xFF;
    lcd_data(0x2B, data, 4);
}

static void lcd_init_manual(void)
{
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_LCD_SCLK,
        .mosi_io_num = PIN_NUM_LCD_MOSI,
        .miso_io_num = PIN_NUM_LCD_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * 40 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_LCD_DC,
        .cs_gpio_num = PIN_NUM_LCD_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &lcd_io));

    lcd_reset_hw();

    lcd_cmd(0xEF);
    lcd_cmd(0xEB);
    { uint8_t d = 0x14; lcd_data(0xEB, &d, 1); }

    lcd_cmd(0xFE);
    lcd_cmd(0xEF);

    { uint8_t d = 0x40; lcd_data(0x84, &d, 1); }
    { uint8_t d = 0xFF; lcd_data(0x85, &d, 1); }
    { uint8_t d = 0xFF; lcd_data(0x86, &d, 1); }
    { uint8_t d = 0xFF; lcd_data(0x87, &d, 1); }
    { uint8_t d = 0x0A; lcd_data(0x88, &d, 1); }
    { uint8_t d = 0x21; lcd_data(0x89, &d, 1); }
    { uint8_t d = 0x00; lcd_data(0x8A, &d, 1); }
    { uint8_t d = 0x80; lcd_data(0x8B, &d, 1); }
    { uint8_t d = 0x01; lcd_data(0x8C, &d, 1); }
    { uint8_t d = 0x01; lcd_data(0x8D, &d, 1); }
    { uint8_t d = 0xFF; lcd_data(0x8E, &d, 1); }
    { uint8_t d = 0xFF; lcd_data(0x8F, &d, 1); }

    { uint8_t d[2] = {0x00, 0x20}; lcd_data(0xB6, d, 2); }
    { uint8_t d = 0x08; lcd_data(0x36, &d, 1); }
    { uint8_t d = 0x05; lcd_data(0x3A, &d, 1); }

    lcd_cmd(0x11);
    vTaskDelay(pdMS_TO_TICKS(120));

    lcd_cmd(0x21);
    lcd_cmd(0x29);
    vTaskDelay(pdMS_TO_TICKS(20));

    lcd_backlight_on();
}

static void lcd_fill_rect(int x, int y, int w, int h, uint16_t color)
{
    if (w <= 0 || h <= 0) return;

    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > LCD_H_RES) w = LCD_H_RES - x;
    if (y + h > LCD_V_RES) h = LCD_V_RES - y;
    if (w <= 0 || h <= 0) return;

    const int chunk_rows = 20;
    int rows = (h < chunk_rows) ? h : chunk_rows;
    uint16_t color_be = color_to_be(color);

    uint16_t *buf = (uint16_t *)heap_caps_malloc(w * rows * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (!buf) {
        ESP_LOGE(TAG, "Sin memoria DMA");
        return;
    }

    for (int i = 0; i < w * rows; i++) {
        buf[i] = color_be;
    }

    int y_pos = y;
    int remaining = h;

    while (remaining > 0) {
        int draw_rows = (remaining < rows) ? remaining : rows;
        lcd_set_window(x, y_pos, x + w - 1, y_pos + draw_rows - 1);
        ESP_ERROR_CHECK(esp_lcd_panel_io_tx_color(lcd_io, 0x2C, buf, w * draw_rows * sizeof(uint16_t)));
        y_pos += draw_rows;
        remaining -= draw_rows;
    }

    free(buf);
}

static void lcd_fill_screen(uint16_t color)
{
    lcd_fill_rect(0, 0, LCD_H_RES, LCD_V_RES, color);
}

static const uint8_t *get_char_bitmap(char c)
{
    if (c >= '0' && c <= '9') return FONT_DIGITS[c - '0'];

    switch (c) {
        case 'A': return FONT_A;
        case 'B': return FONT_B;
        case 'D': return FONT_D;
        case 'E': return FONT_E;
        case 'F': return FONT_F;
        case 'G': return FONT_G;
        case 'H': return FONT_H;
        case 'I': return FONT_I;
        case 'L': return FONT_L;
        case 'M': return FONT_M;
        case 'N': return FONT_N;
        case 'O': return FONT_O;
        case 'P': return FONT_P;
        case 'R': return FONT_R;
        case 'S': return FONT_S;
        case 'T': return FONT_T;
        case 'U': return FONT_U;
        case 'Y': return FONT_Y;
        case ':': return FONT_COLON;
        case '-': return FONT_DASH;
        case '.': return FONT_DOT;
        case ' ': return FONT_SPACE;
        default:  return FONT_SPACE;
    }
}

static void lcd_draw_char(int x, int y, char c, int scale, uint16_t fg, uint16_t bg)
{
    const uint8_t *bitmap = get_char_bitmap(c);

    for (int col = 0; col < 5; col++) {
        uint8_t line = bitmap[col];
        for (int row = 0; row < 7; row++) {
            bool pixel_on = (line >> row) & 0x01;
            lcd_fill_rect(
                x + col * scale,
                y + row * scale,
                scale,
                scale,
                pixel_on ? fg : bg
            );
        }
    }

    lcd_fill_rect(x + 5 * scale, y, scale, 7 * scale, bg);
}

static void lcd_draw_text(int x, int y, const char *text, int scale, uint16_t fg, uint16_t bg)
{
    int cursor_x = x;
    while (*text) {
        lcd_draw_char(cursor_x, y, *text, scale, fg, bg);
        cursor_x += 6 * scale;
        text++;
    }
}

esp_err_t display_init(void)
{
    if (s_display_ready) {
        return ESP_OK;
    }

    lcd_init_manual();
    lcd_fill_screen(COLOR_BLACK);
    s_display_ready = true;
    return ESP_OK;
}

esp_err_t display_show_data(const app_data_t *data)
{
    if (!s_display_ready || !data) {
        return ESP_ERR_INVALID_STATE;
    }

    char line1[32];
    char line2[32];
    char line3[32];
    char line4[32];
    char line5[32];

    lcd_fill_screen(COLOR_BLACK);

    // Barra superior táctil
    lcd_draw_text(6,   4, "PULSO",   2, data->current_screen == SCREEN_PULSE  ? COLOR_GREEN : COLOR_WHITE, COLOR_BLACK);
    lcd_draw_text(62,  4, "IMU",     2, data->current_screen == SCREEN_IMU    ? COLOR_GREEN : COLOR_WHITE, COLOR_BLACK);
    lcd_draw_text(112, 4, "GIRO",    2, data->current_screen == SCREEN_GYRO   ? COLOR_GREEN : COLOR_WHITE, COLOR_BLACK);
    lcd_draw_text(170, 4, "SISTEMA", 2, data->current_screen == SCREEN_SYSTEM ? COLOR_GREEN : COLOR_WHITE, COLOR_BLACK);

    switch (data->current_screen) {
        case SCREEN_PULSE:
            if (data->rtc_valid) {
                snprintf(line1, sizeof(line1), "%04d-%02d-%02d",
                         data->year, data->month, data->day);
                snprintf(line2, sizeof(line2), "%02d:%02d:%02d",
                         data->hour, data->minute, data->second);
            } else {
                snprintf(line1, sizeof(line1), "NO RTC");
                snprintf(line2, sizeof(line2), "--:--:--");
            }

            snprintf(line3, sizeof(line3), "BPM %d", data->bpm_avg);
            snprintf(line4, sizeof(line4), "DEDO %s", data->finger_present ? "SI" : "NO");
            snprintf(line5, sizeof(line5), "RAW %d", data->raw_pulse);

            lcd_draw_text(18, 40,  line1, 3, COLOR_CYAN,   COLOR_BLACK);
            lcd_draw_text(36, 75,  line2, 4, COLOR_WHITE,  COLOR_BLACK);
            lcd_draw_text(25, 125, line3, 5, COLOR_GREEN,  COLOR_BLACK);
            lcd_draw_text(18, 180, line4, 3, COLOR_YELLOW, COLOR_BLACK);
            lcd_draw_text(18, 210, line5, 2, COLOR_GRAY,   COLOR_BLACK);
            break;

        case SCREEN_IMU:
            snprintf(line1, sizeof(line1), "ACELEROMETRO");
            snprintf(line2, sizeof(line2), "AX %.2f", data->ax_g);
            snprintf(line3, sizeof(line3), "AY %.2f", data->ay_g);
            snprintf(line4, sizeof(line4), "AZ %.2f", data->az_g);
            snprintf(line5, sizeof(line5), "TEMP %.1f", data->imu_temp_c);

            lcd_draw_text(18, 40,  line1, 3, COLOR_CYAN,  COLOR_BLACK);
            lcd_draw_text(18, 90,  line2, 3, COLOR_WHITE, COLOR_BLACK);
            lcd_draw_text(18, 125, line3, 3, COLOR_WHITE, COLOR_BLACK);
            lcd_draw_text(18, 160, line4, 3, COLOR_WHITE, COLOR_BLACK);
            lcd_draw_text(18, 200, line5, 3, COLOR_GRAY,  COLOR_BLACK);
            break;

        case SCREEN_GYRO:
            snprintf(line1, sizeof(line1), "GIROSCOPIO");
            snprintf(line2, sizeof(line2), "GX %.2f", data->gx_dps);
            snprintf(line3, sizeof(line3), "GY %.2f", data->gy_dps);
            snprintf(line4, sizeof(line4), "GZ %.2f", data->gz_dps);
            snprintf(line5, sizeof(line5), "FRAME %lu", (unsigned long)data->frame_counter);

            lcd_draw_text(18, 40,  line1, 3, COLOR_CYAN,  COLOR_BLACK);
            lcd_draw_text(18, 90,  line2, 3, COLOR_WHITE, COLOR_BLACK);
            lcd_draw_text(18, 125, line3, 3, COLOR_WHITE, COLOR_BLACK);
            lcd_draw_text(18, 160, line4, 3, COLOR_WHITE, COLOR_BLACK);
            lcd_draw_text(18, 200, line5, 2, COLOR_GRAY,  COLOR_BLACK);
            break;

        case SCREEN_SYSTEM:
        default:
            snprintf(line1, sizeof(line1), "SISTEMA");
            snprintf(line2, sizeof(line2), "RTC %s", data->rtc_valid ? "OK" : "FAIL");
            snprintf(line3, sizeof(line3), "TOUCH %s", data->touch_pressed ? "ON" : "OFF");
            snprintf(line4, sizeof(line4), "X %u Y %u", data->touch_x, data->touch_y);
            snprintf(line5, sizeof(line5), "FRAME %lu", (unsigned long)data->frame_counter);

            lcd_draw_text(18, 40,  line1, 3, COLOR_CYAN,   COLOR_BLACK);
            lcd_draw_text(18, 90,  line2, 3, COLOR_WHITE,  COLOR_BLACK);
            lcd_draw_text(18, 125, line3, 3, COLOR_YELLOW, COLOR_BLACK);
            lcd_draw_text(18, 160, line4, 2, COLOR_GRAY,   COLOR_BLACK);
            lcd_draw_text(18, 200, line5, 2, COLOR_GRAY,   COLOR_BLACK);
            break;
    }

    return ESP_OK;
}
void display_fill(uint16_t color)
{
    lcd_fill_screen(color);
}

void display_draw_pixel(int x, int y, uint16_t color)
{
    if (x < 0 || x >= 240 || y < 0 || y >= 240) {
        return;
    }

    lcd_fill_rect(x, y, 1, 1, color);
}