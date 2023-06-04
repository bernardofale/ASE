#include "display7seg.h"

#define GPIO_OUTPUT_IO_0    32   // GPIO pin for segment A
#define GPIO_OUTPUT_IO_1    33   // GPIO pin for segment B
#define GPIO_OUTPUT_IO_2    25   // GPIO pin for segment C
#define GPIO_OUTPUT_IO_3    26   // GPIO pin for segment D
#define GPIO_OUTPUT_IO_4    27   // GPIO pin for segment E
#define GPIO_OUTPUT_IO_5    14   // GPIO pin for segment F
#define GPIO_OUTPUT_IO_6    12   // GPIO pin for segment G

#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0) | \
                              (1ULL<<GPIO_OUTPUT_IO_1) | \
                              (1ULL<<GPIO_OUTPUT_IO_2) | \
                              (1ULL<<GPIO_OUTPUT_IO_3) | \
                              (1ULL<<GPIO_OUTPUT_IO_4) | \
                              (1ULL<<GPIO_OUTPUT_IO_5) | \
                              (1ULL<<GPIO_OUTPUT_IO_6))

const uint8_t letters[] = {
        0b01110111,  // A
        0b01111100,  // b
        0b00111001,  // C
        0b01011110,  // d
        0b01111001,  // E
        0b01110001,  // F
        0b01111101   // G
    };

const char characters[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G'
};

void display7seg_setup(){
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
}

void display(const char* TAG, int n){
    ESP_LOGI(TAG, "Displaying character: %c", characters[n]); // Log the character

     // Display the letter on the 7-segment display
    gpio_set_level(GPIO_OUTPUT_IO_0, (letters[n] >> 0) & 1);
    gpio_set_level(GPIO_OUTPUT_IO_1, (letters[n] >> 1) & 1);
    gpio_set_level(GPIO_OUTPUT_IO_2, (letters[n] >> 2) & 1);
    gpio_set_level(GPIO_OUTPUT_IO_3, (letters[n] >> 3) & 1);
    gpio_set_level(GPIO_OUTPUT_IO_4, (letters[n] >> 4) & 1);
    gpio_set_level(GPIO_OUTPUT_IO_5, (letters[n] >> 5) & 1);
    gpio_set_level(GPIO_OUTPUT_IO_6, (letters[n] >> 6) & 1);
}