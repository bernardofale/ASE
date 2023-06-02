#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define GPIO_OUTPUT_IO_0    CONFIG_GPIO_OUTPUT_0   // GPIO pin for segment A
#define GPIO_OUTPUT_IO_1    CONFIG_GPIO_OUTPUT_1   // GPIO pin for segment B
#define GPIO_OUTPUT_IO_2    CONFIG_GPIO_OUTPUT_2   // GPIO pin for segment C
#define GPIO_OUTPUT_IO_3    CONFIG_GPIO_OUTPUT_3   // GPIO pin for segment D
#define GPIO_OUTPUT_IO_4    CONFIG_GPIO_OUTPUT_4   // GPIO pin for segment E
#define GPIO_OUTPUT_IO_5    CONFIG_GPIO_OUTPUT_5   // GPIO pin for segment F
#define GPIO_OUTPUT_IO_6    CONFIG_GPIO_OUTPUT_6   // GPIO pin for segment G

#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0) | \
                              (1ULL<<GPIO_OUTPUT_IO_1) | \
                              (1ULL<<GPIO_OUTPUT_IO_2) | \
                              (1ULL<<GPIO_OUTPUT_IO_3) | \
                              (1ULL<<GPIO_OUTPUT_IO_4) | \
                              (1ULL<<GPIO_OUTPUT_IO_5) | \
                              (1ULL<<GPIO_OUTPUT_IO_6))

static const char *TAG = "DISPLAY7SEG APP";

void app_main(void)
{
    //zero-initialize the config structure.
    gpio_config_t io_conf = {};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set, e.g. GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    const uint8_t letters[] = {
        0b01111110,  // A
        0b00110000,  // B
        0b01001110,  // C
        0b01100000,  // D
        0b01001111,  // E
        0b01000111,  // F
        0b01101110   // G
    };

    const char characters[] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G'
    };

    int cnt = 0;
    while(1) {
        if(cnt == 7) cnt = 0;
        ESP_LOGI(TAG, "Displaying character: %c", characters[cnt++]); // Log the character

        // Display the letter on the 7-segment display
        gpio_set_level(GPIO_OUTPUT_IO_0, (letters[cnt] >> 0) & 1);
        gpio_set_level(GPIO_OUTPUT_IO_1, (letters[cnt] >> 1) & 1);
        gpio_set_level(GPIO_OUTPUT_IO_2, (letters[cnt] >> 2) & 1);
        gpio_set_level(GPIO_OUTPUT_IO_3, (letters[cnt] >> 3) & 1);
        gpio_set_level(GPIO_OUTPUT_IO_4, (letters[cnt] >> 4) & 1);
        gpio_set_level(GPIO_OUTPUT_IO_5, (letters[cnt] >> 5) & 1);
        gpio_set_level(GPIO_OUTPUT_IO_6, (letters[cnt] >> 6) & 1);

        vTaskDelay(3000 / portTICK_PERIOD_MS); 
    }
}

