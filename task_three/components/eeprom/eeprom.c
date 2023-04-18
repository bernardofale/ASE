#include "eeprom.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/spi_master.h"


static uint8_t* calculate_instruction_address(uint8_t command, uint16_t address) {
    uint8_t bit8;
    uint8_t real_address;
    bit8 = (address >> 8) & 1;
    command |= bit8 << 3;
    real_address = address & 0xFF;

    uint8_t* data = malloc(2 * sizeof(uint8_t));
    data[0] = command;
    data[1] = real_address;

    return data;
}

esp_err_t spi_25LC040_init(spi_host_device_t masterHostId, int csPin, int sckPin, int mosiPin, int misoPin, int clkSpeedHz, spi_device_handle_t* pDevHandle){

    esp_err_t ret;

    spi_bus_config_t buscfg = {
        .miso_io_num = misoPin,
        .mosi_io_num = mosiPin,
        .sclk_io_num = sckPin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
        .flags = SPI_DEVICE_HALFDUPLEX,
    };

    spi_device_interface_config_t devicecfg = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0,
        .clock_speed_hz = clkSpeedHz,
        .spics_io_num = csPin,
        .queue_size = 1,
    };

    ret = spi_bus_initialize(masterHostId, &buscfg, SPI_DMA_DISABLED);
    ESP_ERROR_CHECK(ret);

    ret = spi_bus_add_device(masterHostId, &devicecfg, pDevHandle);
    ESP_ERROR_CHECK(ret);

    return ret;
}

esp_err_t spi_25LC040_free(spi_host_device_t masterHostId, spi_device_handle_t devHandle){
    esp_err_t ret;

    ret = spi_bus_remove_device(devHandle);
    ESP_ERROR_CHECK(ret);

    ret = spi_bus_free(masterHostId);
    ESP_ERROR_CHECK(ret);

    return ret;
}

esp_err_t spi_25LC040_read_byte(spi_device_handle_t devHandle, uint16_t address, uint8_t* pData){
    esp_err_t ret;

    spi_transaction_t t;
    

    uint8_t *tx_data = calculate_instruction_address(0x03, address);
    memset(&t, 0, sizeof(t));
    
    t.length = 3 * 8;
    t.tx_buffer = tx_data;
    t.rxlength = 8;
    t.rx_buffer = pData;

    ret = spi_device_polling_transmit(devHandle, &t);
    assert(ret == ESP_OK);

    free(tx_data);

    return ret;
}

esp_err_t spi_25LC040_write_byte(spi_device_handle_t devHandle, uint16_t address, uint8_t data){
    esp_err_t ret;

    spi_transaction_t t;

    uint8_t *tx_data = calculate_instruction_address(0x02, address);
    memset(&t, 0, sizeof(t));
    
    t.length = 3 * 8;
    t.tx_buffer = tx_data;
    memccpy(t.tx_buffer + 2, &data, 1, sizeof(data));

    ret = spi_device_polling_transmit(devHandle, &t);
    assert(ret == ESP_OK);

    free(tx_data);

    return ret;
}

esp_err_t spi_25LC040_write_page(spi_device_handle_t devHandle, uint16_t address, const uint8_t* pBuffer, uint8_t size){
    esp_err_t ret;

    spi_transaction_t t;

    uint8_t *tx_data = calculate_instruction_address(0x02, address);
    memset(&t, 0, sizeof(t));
    
    t.length = (2 + size) * 8;
    t.tx_buffer = tx_data;
    memccpy(t.tx_buffer + 2, pBuffer, 1, size);

    ret = spi_device_polling_transmit(devHandle, &t);
    assert(ret == ESP_OK);

    free(tx_data);

    return ret;
}

esp_err_t spi_25LC040_write_enable(spi_device_handle_t devHandle){
    esp_err_t ret;

    spi_transaction_t t;
    uint8_t si = 0x06;
    memset(&t, 0, sizeof(t));
    
    t.length = 8;
    t.tx_buffer = &si;

    ret = spi_device_polling_transmit(devHandle, &t);
    assert(ret == ESP_OK);


    return ret;
}

esp_err_t spi_25LC040_write_disable(spi_device_handle_t devHandle){
    esp_err_t ret;

    spi_transaction_t t;
    uint8_t si = 0x04;
    memset(&t, 0, sizeof(t));
    
    t.length = 8;
    t.tx_buffer = &si;

    ret = spi_device_polling_transmit(devHandle, &t);
    assert(ret == ESP_OK);


    return ret;
}

esp_err_t spi_25LC040_read_status(spi_device_handle_t devHandle, uint8_t* pStatus){
    esp_err_t ret;

    spi_transaction_t t;
    uint8_t si = 0x05;
    memset(&t, 0, sizeof(t));
    
    t.length = 8 * 2;
    t.tx_buffer = &si;
    t.rxlength = 8;
    t.rx_buffer = pStatus;

    ret = spi_device_polling_transmit(devHandle, &t);
    assert(ret == ESP_OK);

    return ret;
}

esp_err_t spi_25LC040_write_status(spi_device_handle_t devHandle, uint8_t status){
    esp_err_t ret;

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    uint8_t data[2] = {0x01, status};

    t.length = 2 * 8;
    t.tx_buffer = data;

    ret = spi_device_polling_transmit(devHandle, &t);
    assert(ret == ESP_OK);

    return ret;
}

