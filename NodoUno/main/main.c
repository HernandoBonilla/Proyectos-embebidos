#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_err.h"

#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"

#include "pulse_sensor.h"
#include "qmi8658.h"
#include "ds3231.h"
#include "i2c_bus.h"
#include "display.h"
#include "touch.h"

#include "app_data.h"

#define TAG "MAIN"

// ============================
// CONFIG WIFI
// ============================
#define WIFI_SSID      "Hernando"
#define WIFI_PASS      "12345678"
#define WIFI_MAX_RETRY 10

static EventGroupHandle_t wifi_event_group;
static const int WIFI_CONNECTED_BIT = BIT0;
static int s_retry_num = 0;

// ============================
// DATA GLOBAL
// ============================
static app_data_t g_data = {0};
static SemaphoreHandle_t data_mutex;

// ============================
// CHECKSUM XOR
// ============================
static uint8_t calc_checksum(const char *data)
{
    uint8_t chk = 0;
    while (*data) {
        chk ^= (uint8_t)(*data++);
    }
    return chk;
}

// ============================
// WIFI EVENTS
// ============================
static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Reintentando conexion WiFi...");
        } else {
            ESP_LOGE(TAG, "No se pudo conectar a WiFi");
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "WiFi conectado, IP obtenida");
        ESP_LOGI(TAG, "IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// ============================
// WIFI INIT
// ============================
static void wifi_init_sta(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    strncpy((char *)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Conectando a WiFi...");
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
}

// ============================
// NTP SYNC
// ============================
static void sync_time_with_ntp(void)
{
    ESP_LOGI(TAG, "Sincronizando hora por NTP...");

    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    ESP_ERROR_CHECK(esp_netif_sntp_init(&config));

    if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(15000)) != ESP_OK) {
        ESP_LOGE(TAG, "Timeout esperando sincronizacion NTP");
        return;
    }

    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    ESP_LOGI(TAG, "Hora NTP obtenida: %04d-%02d-%02d %02d:%02d:%02d",
             timeinfo.tm_year + 1900,
             timeinfo.tm_mon + 1,
             timeinfo.tm_mday,
             timeinfo.tm_hour,
             timeinfo.tm_min,
             timeinfo.tm_sec);
}

// ============================
// COPY SYSTEM TIME TO RTC
// ============================
static esp_err_t rtc_set_from_system_time(void)
{
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    ds3231_time_t rtc_set = {
        .year = timeinfo.tm_year + 1900,
        .month = timeinfo.tm_mon + 1,
        .day = timeinfo.tm_mday,
        .hour = timeinfo.tm_hour,
        .minute = timeinfo.tm_min,
        .second = timeinfo.tm_sec,
        .valid = true
    };

    ESP_LOGI(TAG, "Guardando hora NTP en DS3231...");
    return ds3231_set_time(&rtc_set);
}

// ============================
// TASK: PULSE
// ============================
static void task_pulse(void *pv)
{
    pulse_data_t pulse;

    while (1) {
        pulse_sensor_update();
        pulse_sensor_get_data(&pulse);

        if (xSemaphoreTake(data_mutex, portMAX_DELAY)) {
            g_data.bpm_avg = pulse.bpm_avg;
            g_data.bpm_inst = pulse.bpm_instant;
            g_data.finger_present = pulse.finger_present;
            g_data.raw_pulse = pulse.raw;
            g_data.mv_pulse = pulse.voltage_mv;
            xSemaphoreGive(data_mutex);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ============================
// TASK: IMU
// ============================
static void task_imu(void *pv)
{
    qmi8658_data_t imu;

    while (1) {
        if (qmi8658_read(&imu) == ESP_OK) {
            if (xSemaphoreTake(data_mutex, portMAX_DELAY)) {
                g_data.ax_g = imu.ax_g;
                g_data.ay_g = imu.ay_g;
                g_data.az_g = imu.az_g;

                g_data.gx_dps = imu.gx_dps;
                g_data.gy_dps = imu.gy_dps;
                g_data.gz_dps = imu.gz_dps;

                g_data.imu_temp_c = imu.temp_c;
                xSemaphoreGive(data_mutex);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// ============================
// TASK: RTC
// ============================
static void task_rtc(void *pv)
{
    ds3231_time_t rtc;

    while (1) {
        if (ds3231_get_time(&rtc) == ESP_OK && rtc.valid) {

            ESP_LOGI("RTC", "Fecha/Hora: %04d-%02d-%02d %02d:%02d:%02d",
                     rtc.year, rtc.month, rtc.day,
                     rtc.hour, rtc.minute, rtc.second);

            if (xSemaphoreTake(data_mutex, portMAX_DELAY)) {
                g_data.year = rtc.year;
                g_data.month = rtc.month;
                g_data.day = rtc.day;
                g_data.hour = rtc.hour;
                g_data.minute = rtc.minute;
                g_data.second = rtc.second;
                g_data.rtc_valid = true;
                xSemaphoreGive(data_mutex);
            }
        } else {
            ESP_LOGE("RTC", "No se pudo leer el RTC");

            if (xSemaphoreTake(data_mutex, portMAX_DELAY)) {
                g_data.rtc_valid = false;
                xSemaphoreGive(data_mutex);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ============================
// TASK: SERIAL
// ============================
static void task_serial(void *pv)
{
    char payload[320];
    app_data_t local;

    while (1) {
        if (xSemaphoreTake(data_mutex, portMAX_DELAY)) {
            local = g_data;
            g_data.frame_counter++;
            local.frame_counter = g_data.frame_counter;
            xSemaphoreGive(data_mutex);
        }

        if (local.rtc_valid) {
            snprintf(payload, sizeof(payload),
                     "$NODO1,%04d-%02d-%02d,%02d:%02d:%02d,%d,%d,%d,%d,%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%lu",
                     local.year,
                     local.month,
                     local.day,
                     local.hour,
                     local.minute,
                     local.second,
                     local.bpm_avg,
                     local.bpm_inst,
                     local.finger_present ? 1 : 0,
                     local.raw_pulse,
                     local.mv_pulse,
                     local.ax_g,
                     local.ay_g,
                     local.az_g,
                     local.gx_dps,
                     local.gy_dps,
                     local.gz_dps,
                     local.imu_temp_c,
                     (unsigned long)local.frame_counter);
        } else {
            snprintf(payload, sizeof(payload),
                     "$NODO1,NO_RTC,NO_RTC,%d,%d,%d,%d,%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%lu",
                     local.bpm_avg,
                     local.bpm_inst,
                     local.finger_present ? 1 : 0,
                     local.raw_pulse,
                     local.mv_pulse,
                     local.ax_g,
                     local.ay_g,
                     local.az_g,
                     local.gx_dps,
                     local.gy_dps,
                     local.gz_dps,
                     local.imu_temp_c,
                     (unsigned long)local.frame_counter);
        }

        printf("%s\n", payload);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// ============================
// TASK: DISPLAY
// ============================
static void task_display(void *pv)
{
    app_data_t local;

    while (1) {
        if (xSemaphoreTake(data_mutex, portMAX_DELAY)) {
            local = g_data;
            xSemaphoreGive(data_mutex);
        }

        display_show_data(&local);

        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

// ============================
// TASK: POWER
// ============================
static void task_power(void *pv)
{
    while (1) {
        if (xSemaphoreTake(data_mutex, portMAX_DELAY)) {
            if (!g_data.finger_present && g_data.bpm_avg == 0) {
                ESP_LOGI(TAG, "Modo IDLE");
            } else {
                ESP_LOGI(TAG, "Modo NORMAL");
            }
            xSemaphoreGive(data_mutex);
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}


static void task_touch(void *pv)
{
    touch_data_t touch;
    bool prev_pressed = false;

    while (1) {
        if (touch_read(&touch) == ESP_OK) {
            if (xSemaphoreTake(data_mutex, portMAX_DELAY)) {
                g_data.touch_pressed = touch.pressed;
                g_data.touch_x = touch.x;
                g_data.touch_y = touch.y;

                if (touch.pressed && !prev_pressed) {
                    if (touch.y <= 40) {
                        if (touch.x < 60) {
                            g_data.current_screen = SCREEN_PULSE;
                        } else if (touch.x < 110) {
                            g_data.current_screen = SCREEN_IMU;
                        } else if (touch.x < 170) {
                            g_data.current_screen = SCREEN_GYRO;
                        } else {
                            g_data.current_screen = SCREEN_SYSTEM;
                        }
                    }
                }

                xSemaphoreGive(data_mutex);
            }

            prev_pressed = touch.pressed;
        }

        vTaskDelay(pdMS_TO_TICKS(80));
    }
}
// ============================
// APP MAIN
// ============================
void app_main(void)
{
    ESP_LOGI(TAG, "Init system...");

    data_mutex = xSemaphoreCreateMutex();
    if (data_mutex == NULL) {
        ESP_LOGE(TAG, "No se pudo crear data_mutex");
        return;
    }

    g_data.current_screen = SCREEN_PULSE;

    // Inicializar buses
    ESP_ERROR_CHECK(app_i2c_bus_init());

    // Inicializar módulos
    ESP_ERROR_CHECK(pulse_sensor_init());

    // Bus I2C interno: IMU
   
    ESP_ERROR_CHECK(qmi8658_init(app_i2c_bus_get_internal_handle()));
    ESP_ERROR_CHECK(touch_init(app_i2c_bus_get_internal_handle()));
    ESP_ERROR_CHECK(ds3231_init(app_i2c_bus_get_external_handle()));
    ESP_ERROR_CHECK(display_init());

    // WiFi + NTP + guardar hora real en RTC
    wifi_init_sta();
    sync_time_with_ntp();
    ESP_ERROR_CHECK(rtc_set_from_system_time());

    // Crear tareas
    xTaskCreate(task_pulse,   "task_pulse",   4096, NULL, 5, NULL);
    xTaskCreate(task_imu,     "task_imu",     4096, NULL, 5, NULL);
    xTaskCreate(task_rtc,     "task_rtc",     3072, NULL, 4, NULL);
    xTaskCreate(task_touch,   "task_touch",   3072, NULL, 4, NULL);
    xTaskCreate(task_serial,  "task_serial",  4096, NULL, 3, NULL);
    xTaskCreate(task_display, "task_display", 4096, NULL, 3, NULL);
    xTaskCreate(task_power,   "task_power",   2048, NULL, 2, NULL);

    ESP_LOGI(TAG, "Sistema iniciado correctamente");
}

