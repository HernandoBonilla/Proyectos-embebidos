#include "pulse_sensor.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define TAG "PULSE"

// HW-827 en GPIO17 = ADC2_CH6
#define PULSE_ADC_UNIT           ADC_UNIT_2
#define PULSE_ADC_CHANNEL        ADC_CHANNEL_6

#define SAMPLE_PERIOD_MS         10
#define STARTUP_IGNORE_SAMPLES   250

#define REFRACTORY_MS            500
#define MIN_BPM                  45
#define MAX_BPM                  180
#define MIN_INTERVAL_MS          (60000 / MAX_BPM)
#define MAX_INTERVAL_MS          (60000 / MIN_BPM)

#define MIN_ENV_FOR_VALID        10.0f
#define THRESHOLD_FACTOR         0.75f
#define RELEASE_FACTOR           0.45f

#define NO_FINGER_ENV_THRESHOLD  8.0f
#define BEAT_TIMEOUT_MS          1500

#define BPM_AVG_SIZE             5

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle = NULL;
static bool do_calibration = false;

static pulse_data_t g_pulse = {0};

static int sample_counter = 0;
static float dc_estimate = 0.0f;
static float ac_signal = 0.0f;
static float filtered = 0.0f;
static float envelope = 0.0f;
static float prev_filtered = 0.0f;

static bool above_threshold = false;
static int64_t last_beat_ms = 0;

static int bpm_history[BPM_AVG_SIZE] = {0};
static int bpm_history_count = 0;
static int bpm_history_index = 0;

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

static void clear_bpm_history(void)
{
    for (int i = 0; i < BPM_AVG_SIZE; i++) {
        bpm_history[i] = 0;
    }
    bpm_history_count = 0;
    bpm_history_index = 0;
}

esp_err_t pulse_sensor_init(void)
{
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

    memset(&g_pulse, 0, sizeof(g_pulse));
    return ESP_OK;
}

esp_err_t pulse_sensor_update(void)
{
    int raw = 0;
    int voltage_mv = 0;

    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, PULSE_ADC_CHANNEL, &raw));

    if (do_calibration) {
        adc_cali_raw_to_voltage(cali_handle, raw, &voltage_mv);
    } else {
        voltage_mv = (raw * 3100) / 4095;
    }

    sample_counter++;

    g_pulse.raw = raw;
    g_pulse.voltage_mv = voltage_mv;

    if (sample_counter <= STARTUP_IGNORE_SAMPLES) {
        dc_estimate = 0.99f * dc_estimate + 0.01f * (float)voltage_mv;
        g_pulse.filtered = 0.0f;
        g_pulse.envelope = 0.0f;
        g_pulse.bpm_instant = 0;
        g_pulse.bpm_avg = 0;
        g_pulse.finger_present = false;
        return ESP_OK;
    }

    dc_estimate = 0.995f * dc_estimate + 0.005f * (float)voltage_mv;
    ac_signal = (float)voltage_mv - dc_estimate;
    filtered = 0.84f * filtered + 0.16f * ac_signal;

    float abs_signal = (filtered >= 0.0f) ? filtered : -filtered;
    envelope = 0.985f * envelope + 0.015f * abs_signal;

    float threshold = envelope * THRESHOLD_FACTOR;
    int64_t now_ms = esp_log_timestamp();

    bool finger_present = (envelope > NO_FINGER_ENV_THRESHOLD);

    if (!finger_present || (last_beat_ms != 0 && (now_ms - last_beat_ms) > BEAT_TIMEOUT_MS)) {
        g_pulse.bpm_instant = 0;
        g_pulse.bpm_avg = 0;
        above_threshold = false;
        clear_bpm_history();

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
            if (last_beat_ms != 0) {
                if (delta_ms >= MIN_INTERVAL_MS && delta_ms <= MAX_INTERVAL_MS) {
                    g_pulse.bpm_instant = (int)(60000 / delta_ms);

                    bpm_history[bpm_history_index] = g_pulse.bpm_instant;
                    bpm_history_index = (bpm_history_index + 1) % BPM_AVG_SIZE;

                    if (bpm_history_count < BPM_AVG_SIZE) {
                        bpm_history_count++;
                    }

                    g_pulse.bpm_avg = average_int_array(bpm_history, bpm_history_count);
                }
            }

            last_beat_ms = now_ms;
            above_threshold = true;
        }
    }

    if (above_threshold && filtered < (threshold * RELEASE_FACTOR)) {
        above_threshold = false;
    }

    prev_filtered = filtered;

    g_pulse.filtered = filtered;
    g_pulse.envelope = envelope;
    g_pulse.finger_present = finger_present;

    return ESP_OK;
}

void pulse_sensor_get_data(pulse_data_t *out)
{
    if (!out) {
        return;
    }
    *out = g_pulse;
}