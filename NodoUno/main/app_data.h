#ifndef APP_DATA_H
#define APP_DATA_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    SCREEN_PULSE = 0,
    SCREEN_IMU,
    SCREEN_GYRO,
    SCREEN_SYSTEM
} screen_id_t;

typedef struct {
    int bpm_avg;
    int bpm_inst;
    bool finger_present;
    int raw_pulse;
    int mv_pulse;

    float ax_g;
    float ay_g;
    float az_g;

    float gx_dps;
    float gy_dps;
    float gz_dps;

    float imu_temp_c;

    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    bool rtc_valid;


    float pecho_ax_g;
    float pecho_ay_g;
    float pecho_az_g;

    float pecho_gx_dps;
    float pecho_gy_dps;
    float pecho_gz_dps;

    float pecho_pitch_deg;
    float pecho_roll_deg;
    float pecho_accel_total_g;
    float pecho_gyro_total_dps;

    uint8_t pecho_estado;
    uint8_t pecho_caida;
    uint32_t pecho_frame;

    uint32_t frame_counter;

    bool touch_pressed;
    uint16_t touch_x;
    uint16_t touch_y;
    screen_id_t current_screen;

    

  
} app_data_t;

#endif