idf_component_register(
    SRCS "main.c"
        "pantalla.c"
         "pulse/pulse_sensor.c"
         "imu/qmi8658.c"
         "rtc/ds3231.c"
         "i2c_bus/i2c_bus.c"
         "display/display.c"
         "touch/touch.c"
    INCLUDE_DIRS "." "pulse" "imu" "rtc" "i2c_bus" "display" "touch"
    REQUIRES esp_adc 
    esp_driver_i2c 
    driver 
    esp_lcd 
    esp_wifi 
    esp_event 
    esp_netif 
    nvs_flash 
    lwip
    
)