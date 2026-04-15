##
# @file CMakeLists.txt
# @brief Registro del componente principal del proyecto en ESP-IDF.
#
# Este archivo define los archivos fuente, directorios de inclusión
# y dependencias necesarias para construir el componente.
##

##
# @brief Registra un componente en el sistema de compilación de ESP-IDF.
#
# Este componente incluye los módulos principales para:
# - Comunicación I2C
# - Manejo del sensor MPU6050
# - Detección de movimiento
#
idf_component_register(
    SRCS 
        "main.c"             ##< Punto de entrada principal de la aplicación
        "i2c_bus.c"          ##< Implementación de la comunicación I2C
        "mpu6050.c"          ##< Driver del sensor MPU6050
        "motion_detector.c"  ##< Lógica de detección de movimiento
    
    INCLUDE_DIRS 
        "."                  ##< Directorio de cabeceras del componente
    
    REQUIRES 
        esp_driver_i2c       ##< Dependencia del driver I2C de ESP-IDF
)