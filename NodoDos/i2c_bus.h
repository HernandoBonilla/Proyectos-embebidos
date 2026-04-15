#ifndef I2C_BUS_H
#define I2C_BUS_H

#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file i2c_bus.h
 * @brief Interfaz para la inicialización y acceso al bus I2C.
 *
 * Este módulo encapsula la configuración del bus I2C en modo maestro
 * y proporciona funciones para su inicialización y acceso desde otros
 * módulos del sistema.
 */

/**
 * @brief Inicializa el bus I2C.
 *
 * Configura el bus I2C con los parámetros definidos en la implementación.
 * Si el bus ya fue inicializado previamente, la función no realiza ninguna
 * acción adicional.
 *
 * @return
 * - ESP_OK si la inicialización fue exitosa o ya estaba inicializado
 * - Código de error en caso de fallo
 */
esp_err_t app_i2c_init(void);

/**
 * @brief Obtiene el manejador del bus I2C.
 *
 * Permite a otros módulos acceder al bus I2C previamente inicializado.
 *
 * @return
 * - Manejador del bus I2C si está inicializado
 * - NULL si el bus no ha sido inicializado
 */
i2c_master_bus_handle_t app_i2c_get_handle(void);

#ifdef __cplusplus
}
#endif

#endif