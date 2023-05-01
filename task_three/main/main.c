#include <stdio.h>
#include "eeprom.h"
#include "esp_log.h"
#include "temp_sensor_TC74.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_system.h"
#include "freertos/semphr.h"

/* Number of values */
#define NUM_VALUES 3

/* SPI definitions for EEPROM */
#define SPI_MASTER_HOST SPI3_HOST
#define PIN_SPI_SS 5
#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK 18
#define CLK_SPEED 1000000
spi_device_handle_t spi;

/* I2C definitions for TC74 */
#define I2C_MASTER_SCL_IO           18              /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           19              /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0               /*!< I2C master i2c port number, the number of i2c peripheral 
                                                    interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          50000           /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0               /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0               /*!< I2C master doesn't need buffer */
#define GPIO_PULLUP_ENABLE          1
#define I2C_MASTER_TIMEOUT_MS       1000
#define TC74_SENSOR_ADDR            0x4D            /*!< Slave address of the TC74 sensor */
int i2c_master_port = I2C_MASTER_NUM;

/* Thread functions prototypes */
void temp_aq( void *pvParameters );
void calc_mean( void *pvParameters );
void fill_mem( void *pvParameters );

/* Semaphore definition */
SemaphoreHandle_t temp_to_mean;
SemaphoreHandle_t mean_to_mem;

/* Create 8 bit integer array for value storing */
uint8_t vals[NUM_VALUES];

/* Create static mean value for eeprom writing */
uint8_t mean;

void app_main(void)
{

    /* TC74 initialization */
    tc74_init(i2c_master_port, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, I2C_MASTER_FREQ_HZ);

    /* EEPROM initializaion*/
    spi_25LC040_init(SPI_MASTER_HOST, PIN_SPI_SS, PIN_NUM_CLK, PIN_NUM_MOSI, PIN_NUM_MISO, CLK_SPEED, &spi);

    /* Semaphore creation */
    temp_to_mean = xSemaphoreCreateBinary();
    if(temp_to_mean == NULL)
    {
        ESP_LOGI("Semaphore", "Semaphore not created");
    }

    mean_to_mem = xSemaphoreCreateBinary();
    if(mean_to_mem == NULL)
    {
        ESP_LOGI("Semaphore", "Semaphore not created");
    }

    /* Task creation */
    xTaskCreate(&temp_aq, "temp_aquisition", 2048, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(&calc_mean, "calculate_mean", 2048, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(&fill_mem, "fill_memory", 2048, NULL, tskIDLE_PRIORITY, NULL);
}

void temp_aq(void *pvParameters)
{

    while(1)
    {
        ESP_LOGI("Temperature", "Temperature reading!");
        for (int i = 0; i < NUM_VALUES; i++)
        {
            tc74_wakeup_and_read_temp(i2c_master_port, TC74_SENSOR_ADDR, I2C_MASTER_TIMEOUT_MS, &vals[i]);
            ESP_LOGI("Temperature", "Temperature: %d", vals[i]);
            vTaskDelay(pdMS_TO_TICKS(100));
            
        }
        tc74_standby(i2c_master_port, TC74_SENSOR_ADDR, I2C_MASTER_TIMEOUT_MS);
        xSemaphoreGive(temp_to_mean);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void calc_mean(void *pvParameters)
{
    uint8_t sum;
    while(1)
    {
        xSemaphoreTake(temp_to_mean, portMAX_DELAY);
        sum = 0;
        for (int i = 0; i < NUM_VALUES; i++)
        {
            sum += vals[i];
        }
        mean = (uint8_t) sum / NUM_VALUES;
        xSemaphoreGive(mean_to_mem);
    }
}

void fill_mem(void *pvParameters)
{
    bool full = false;
    while(1)
    {
        xSemaphoreTake(mean_to_mem, portMAX_DELAY);

        spi_25LC040_write_enable(spi);
        spi_25LC040_write_status(spi, 0x00);
        ESP_LOGI("Writing to EEPROM", "Mean value: %d", mean);
        spi_25LC040_write_byte(spi, 0x00, mean);
        spi_25LC040_write_disable(spi);

        uint8_t inSens;
        spi_25LC040_read_byte(spi, 0x00, &inSens);
        ESP_LOGI("Retrieved from EEPROM", "Mean value: %d", inSens);

        if(full == true){
            ESP_LOGI("EEPROM", "EEPROM is full");
            // Dump all EEPROM values
        }
    }
}
