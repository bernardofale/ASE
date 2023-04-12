#include "temp_sensor_TC74.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c.h"

#define I2C_MASTER_TX_BUF_DISABLE   0               /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0               /*!< I2C master doesn't need buffer */
#define GPIO_PULLUP_ENABLE          1
#define TC74_SENSOR_ADDR            0x4D            /*!< Slave address of the TC74 sensor */

esp_err_t tc74_init(i2c_port_t i2cPort, int sdaPin, int sclPin, uint32_t clkSpeedHz){

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sdaPin,
        .scl_io_num = sclPin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = clkSpeedHz,
    };

    i2c_param_config(i2cPort, &conf);

    return i2c_driver_install(i2cPort, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

esp_err_t tc_74_free(i2c_port_t i2cPort){
    return i2c_driver_delete(i2cPort);
}