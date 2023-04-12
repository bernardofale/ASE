#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "esp_log.h"


#define SPI_MASTER_HOST SPI3_HOST

#define PIN_SPI_SS 5
#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK 18

void spi_tx_data(spi_device_handle_t spi, const uint8_t *data, size_t len){
    esp_err_t ret;
    spi_transaction_t t;

    if(len <= 0){
        return;
    }
    memset(&t, 0, sizeof(t));
    t.length = len * 8;
    t.tx_buffer = data;
    ret = spi_device_polling_transmit(spi, &t);
    assert(ret == ESP_OK);
}

void spi_tx_rx_data(spi_device_handle_t spi, const uint8_t *tx_data, size_t tx_len, uint8_t *rx_data, size_t rx_len){
    esp_err_t ret;
    spi_transaction_t t;

    if(len <= 0){
        return;
    }
    memset(&t, 0, sizeof(t));
    t.length = (tx_len + rx_len) * 8;
    t.tx_buffer = tx_data;
    t.rxlength = rx_len * 8;
    t.rx_buffer = rx_data;
    ret = spi_device_polling_transmit(spi, &t);
    assert(ret == ESP_OK);
}
void app_main(void)
{
    esp_err_t ret;
    spi_device_handle_t spi;

    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
        .flags = SPICOMMON_BUSFLAG_MASTER,
    };

    spi_device_interface_config_t devicecfg = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0,
        .clock_speed_hz = 1000000,
        .spics_io_num = PIN_SPI_SS,
        .queue_size = 1,
    };

    ret = spi_bus_initialize(SPI_MASTER_HOST, &buscfg, SPI_DMA_DISABLED);
    ESP_ERROR_CHECK(ret);

    ret = spi_bus_add_device(SPI_MASTER_HOST, &devicecfg, &spi);
    ESP_ERROR_CHECK(ret);

    uint8_t tx_data[2] = {0x01, 0x0C};

    spi_tx_data(spi, tx_data, sizeof(tx_data));

    uint8_t tx_data2 = 0x05;
    uint8_t rx_data;

    while(1){
        spi_tx_rx_data(spi, &tx_data2, sizeof(tx_data2), &rx_data, sizeof(rx_data));
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    
}
