/**
 * @file pulse_sensor.c
 * @brief Lectura y procesamiento de señal de pulso (sensor HW827) usando ADC.
 *
 * Este módulo implementa:
 * - Lectura del ADC (modo oneshot)
 * - Filtrado de señal (DC + AC)
 * - Detección de envolvente
 * - Umbral adaptativo
 * - Detección de latidos (BPM)
 * - Validación de presencia de dedo
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

/** @brief Etiqueta de log */
#define TAG "HW827"

/** @name Configuración ADC */
/**@{ */
#define PULSE_ADC_UNIT           ADC_UNIT_2      /**< Unidad ADC usada */
#define PULSE_ADC_CHANNEL        ADC_CHANNEL_6   /**< Canal ADC (GPIO17 en ESP32-S3) */
/**@} */

/** @name Temporización */
/**@{ */
#define SAMPLE_PERIOD_MS         10              /**< Periodo de muestreo (100 Hz) */
#define REPORT_PERIOD_MS         1000            /**< Periodo de reporte */
#define STARTUP_IGNORE_SAMPLES   250             /**< Muestras ignoradas al inicio */
/**@} */

/** @name Detección de latidos */
/**@{ */
#define REFRACTORY_MS            500             /**< Tiempo mínimo entre latidos */
#define MIN_BPM                  45
#define MAX_BPM                  180
#define MIN_INTERVAL_MS          (60000 / MAX_BPM)
#define MAX_INTERVAL_MS          (60000 / MIN_BPM)
/**@} */

/** @name Calidad de señal */
/**@{ */
#define MIN_ENV_FOR_VALID        10.0f           /**< Nivel mínimo de señal */
#define THRESHOLD_FACTOR         0.75f           /**< Factor del umbral adaptativo */
#define RELEASE_FACTOR           0.45f           /**< Factor de liberación */
/**@} */

/** @name Detección de dedo */
/**@{ */
#define NO_FINGER_ENV_THRESHOLD  8.0f            /**< Umbral de ausencia de dedo */
#define BEAT_TIMEOUT_MS          1500            /**< Timeout sin latidos */
/**@} */

/** @brief Tamaño del buffer para promedio de BPM */
#define BPM_AVG_SIZE             5

/** @brief Manejador ADC */
static adc_oneshot_unit_handle_t adc_handle;

/** @brief Manejador de calibración ADC */
static adc_cali_handle_t cali_handle = NULL;

/** @brief Indica si se usa calibración */
static bool do_calibration = false;

/**
 * @brief Inicializa la calibración del ADC.
 *
 * Intenta primero el esquema "curve fitting" y luego "line fitting".
 *
 * @param unit Unidad ADC
 * @param atten Atenuación
 * @param out_handle Manejador de calibración
 * @return true si la calibración está disponible
 */
static bool adc_calibration_init(adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_curve_fitting(&cali_config, &handle) == ESP_OK) {
        calibrated = true;
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        if (adc_cali_create_scheme_line_fitting(&cali_config, &handle) == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    return calibrated;
}

/**
 * @brief Calcula el promedio de un arreglo de enteros.
 *
 * @param arr Arreglo
 * @param count Número de elementos
 * @return Promedio entero
 */
static int average_int_array(const int *arr, int count)
{
    if (count <= 0) {
        return 0;
    }

    int sum = 0;
    for (int i = 0; i < count; i++) {
        sum += arr[i];
    }
    return sum / count;
}

/**
 * @brief Limpia el historial de BPM.
 *
 * @param arr Arreglo de historial
 * @param size Tamaño
 * @param count Contador
 * @param index Índice circular
 */
static void clear_bpm_history(int *arr, int size, int *count, int *index)
{
    for (int i = 0; i < size; i++) {
        arr[i] = 0;
    }
    *count = 0;
    *index = 0;
}

/**
 * @brief Función principal de la aplicación.
 *
 * Implementa:
 * - Inicialización del ADC
 * - Adquisición de señal
 * - Filtrado (DC + AC)
 * - Cálculo de envolvente
 * - Umbral adaptativo
 * - Detección de latidos
 * - Cálculo de BPM
 * - Detección de ausencia de dedo
 */
void app_main(void)
{
    // Inicialización ADC
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = PULSE_ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t chan_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, PULSE_ADC_CHANNEL, &chan_config));

    do_calibration = adc_calibration_init(PULSE_ADC_UNIT, ADC_ATTEN_DB_12, &cali_handle);
    ESP_LOGI(TAG, "Calibracion ADC: %s", do_calibration ? "SI" : "NO");

    /** Variables internas del procesamiento */
    int raw = 0;
    int voltage_mv = 0;
    int sample_counter = 0;

    float dc_estimate = 0.0f;
    float ac_signal = 0.0f;
    float filtered = 0.0f;
    float envelope = 0.0f;
    float prev_filtered = 0.0f;

    bool above_threshold = false;
    bool finger_present = false;

    int64_t last_beat_ms = 0;
    int64_t last_report_ms = 0;

    int bpm_instant = 0;
    int bpm_display = 0;

    int bpm_history[BPM_AVG_SIZE] = {0};
    int bpm_history_count = 0;
    int bpm_history_index = 0;

    while (1) {
        sample_counter++;

        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, PULSE_ADC_CHANNEL, &raw));

        if (do_calibration) {
            adc_cali_raw_to_voltage(cali_handle, raw, &voltage_mv);
        } else {
            voltage_mv = (raw * 3100) / 4095;
        }

        // Ignorar inicio
        if (sample_counter <= STARTUP_IGNORE_SAMPLES) {
            dc_estimate = 0.99f * dc_estimate + 0.01f * (float)voltage_mv;
            vTaskDelay(pdMS_TO_TICKS(SAMPLE_PERIOD_MS));
            continue;
        }

        /** ================= FILTRADO ================= */

        dc_estimate = 0.995f * dc_estimate + 0.005f * (float)voltage_mv;
        ac_signal = (float)voltage_mv - dc_estimate;
        filtered = 0.84f * filtered + 0.16f * ac_signal;

        float abs_signal = (filtered >= 0.0f) ? filtered : -filtered;
        envelope = 0.985f * envelope + 0.015f * abs_signal;

        float threshold = envelope * THRESHOLD_FACTOR;
        int64_t now_ms = esp_log_timestamp();

        /** ================= DETECCION ================= */

        finger_present = (envelope > NO_FINGER_ENV_THRESHOLD);

        if (!finger_present || (last_beat_ms != 0 && (now_ms - last_beat_ms) > BEAT_TIMEOUT_MS)) {
            bpm_instant = 0;
            bpm_display = 0;
            above_threshold = false;
            clear_bpm_history(bpm_history, BPM_AVG_SIZE, &bpm_history_count, &bpm_history_index);

            if (!finger_present) {
                last_beat_ms = 0;
            }
        }

        if (finger_present &&
            !above_threshold &&
            envelope > MIN_ENV_FOR_VALID &&
            prev_filtered <= threshold &&
            filtered > threshold) {

            int64_t delta_ms = now_ms - last_beat_ms;

            if (last_beat_ms == 0 || delta_ms > REFRACTORY_MS) {
                if (last_beat_ms != 0 &&
                    delta_ms >= MIN_INTERVAL_MS &&
                    delta_ms <= MAX_INTERVAL_MS) {

                    bpm_instant = (int)(60000 / delta_ms);

                    bpm_history[bpm_history_index] = bpm_instant;
                    bpm_history_index = (bpm_history_index + 1) % BPM_AVG_SIZE;

                    if (bpm_history_count < BPM_AVG_SIZE) {
                        bpm_history_count++;
                    }

                    bpm_display = average_int_array(bpm_history, bpm_history_count);
                }

                last_beat_ms = now_ms;
                above_threshold = true;
            }
        }

        if (above_threshold && filtered < (threshold * RELEASE_FACTOR)) {
            above_threshold = false;
        }

        prev_filtered = filtered;

        /** ================= LOG ================= */
        if ((now_ms - last_report_ms) >= REPORT_PERIOD_MS) {
            last_report_ms = now_ms;

            ESP_LOGI(TAG,
                     "raw=%d voltage=%d mV filt=%.2f env=%.2f thr=%.2f bpm_inst=%d bpm_avg=%d finger=%s",
                     raw,
                     voltage_mv,
                     filtered,
                     envelope,
                     threshold,
                     bpm_instant,
                     bpm_display,
                     finger_present ? "YES" : "NO");
        }

        vTaskDelay(pdMS_TO_TICKS(SAMPLE_PERIOD_MS));
    }
}