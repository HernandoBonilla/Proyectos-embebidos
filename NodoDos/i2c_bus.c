#include "i2c_bus.h"

/** @brief Pin GPIO usado para la línea SDA del bus I2C */
#define I2C_SDA_IO     21

/** @brief Pin GPIO usado para la línea SCL del bus I2C */
#define I2C_SCL_IO     22

/** @brief Puerto I2C utilizado */
#define I2C_PORT_NUM   0

/** @brief Frecuencia del bus I2C en Hz */
#define I2C_FREQ_HZ    400000

/** 
 * @brief Manejador estático del bus I2C.
 * 
 * Se mantiene como variable estática para asegurar que solo exista
 * una instancia del bus en toda la aplicación.
 */
static i2c_master_bus_handle_t s_i2c_bus = NULL;

/**
 * @brief Inicializa el bus I2C en modo maestro.
 * 
 * Configura los pines, la frecuencia y otros parámetros del bus I2C.
 * Si el bus ya fue inicializado previamente, la función no realiza
 * ninguna acción adicional.
 * 
 * @return
 * - ESP_OK si la inicialización fue exitosa o ya estaba inicializado
 * - Código de error en caso de fallo
 */
esp_err_t app_i2c_init(void)
{
    if (s_i2c_bus != NULL) {
        return ESP_OK;
    }

    /**
     * @brief Configuración del bus I2C maestro.
     */
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_PORT_NUM,                  /**< Puerto I2C */
        .sda_io_num = I2C_SDA_IO,                  /**< Pin SDA */
        .scl_io_num = I2C_SCL_IO,                  /**< Pin SCL */
        .clk_source = I2C_CLK_SRC_DEFAULT,         /**< Fuente de reloj */
        .glitch_ignore_cnt = 7,                    /**< Filtro de glitches */
        .flags.enable_internal_pullup = true,      /**< Habilita pull-ups internos */
    };

    return i2c_new_master_bus(&bus_cfg, &s_i2c_bus);
}

/**
 * @brief Obtiene el manejador del bus I2C.
 * 
 * Permite a otros módulos acceder al bus previamente inicializado.
 * 
 * @return i2c_master_bus_handle_t Manejador del bus I2C
 *         o NULL si no ha sido inicializado.
 */
i2c_master_bus_handle_t app_i2c_get_handle(void)
{
    return s_i2c_bus;
}