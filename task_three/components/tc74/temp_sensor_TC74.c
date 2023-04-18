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

esp_err_t tc74_standby(i2c_port_t i2cPort, uint8_t sensAddr, TickType_t timeOut){
    uint8_t data = 0x01;
    return i2c_master_write_to_device(i2cPort, sensAddr, &data, 1, timeOut);
}

esp_err_t tc74_wakeup(i2c_port_t i2cPort, uint8_t sensAddr, TickType_t timeOut){
    uint8_t data = 0x00;
    return i2c_master_write_to_device(i2cPort, sensAddr, &data, 1, timeOut);
}

bool tc74_is_temperature_ready(i2c_port_t i2cPort, uint8_t sensAddr, TickType_t timeOut){
    uint8_t data = 0x00;
    i2c_master_read_from_device(i2cPort, sensAddr, &data, 1, timeOut);
    return (data & 0x80) == 0x80;
}

esp_err_t tc74_wakeup_and_read_temp(i2c_port_t i2cPort, uint8_t sensAddr, TickType_t timeOut, uint8_t* pData){
    esp_err_t err = tc74_wakeup(i2cPort, sensAddr, timeOut);
    if(err != ESP_OK){
        return err;
    }
    //while(!tc74_is_temperature_ready(i2cPort, sensAddr, timeOut)){
    //    vTaskDelay(10 / portTICK_PERIOD_MS);
    //}
    return i2c_master_read_from_device(i2cPort, sensAddr, pData, 1, timeOut);
}

esp_err_t tc74_read_temp_after_cfg(i2c_port_t i2cPort, uint8_t sensAddr, TickType_t timeOut, uint8_t* pData){
    esp_err_t err = tc74_wakeup(i2cPort, sensAddr, timeOut);
    if(err != ESP_OK){
        return err;
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
    return i2c_master_read_from_device(i2cPort, sensAddr, pData, 1, timeOut);
}

esp_err_t tc74_read_temp_after_temp(i2c_port_t i2cPort, uint8_t sensAddr, TickType_t timeOut, uint8_t* pData){
    return i2c_master_read_from_device(i2cPort, sensAddr, pData, 1, timeOut);
}