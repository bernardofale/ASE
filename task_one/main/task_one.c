/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_console.h"


static const char *TAG = "example";

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO CONFIG_BLINK_GPIO
#define PROMPT_STR CONFIG_IDF_TARGET

#define GPIO_INPUT_IO_0 GPIO_NUM_15

static uint8_t s_led_state = 0;
static int input_freq;

void signal_gen( void *pvParameters );
void signal_acq( void *pvParameters );

static void blink_led(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(BLINK_GPIO, s_led_state);
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}
uint32_t period;
void app_main(void)
{   
    ESP_LOGI(TAG, "Input the generated frequency:\n");
    scanf("%d", &input_freq);
    ESP_LOGI(TAG, "%d", input_freq);
    //period = 1000/input_freq;

    /* Configure the peripheral according to the LED type */
    gpio_set_direction(GPIO_INPUT_IO_0, GPIO_MODE_INPUT);
    configure_led();

    ESP_LOGI(TAG, "Choose the frequency of the square signal (Hz): ");
    while((input_freq < 1) || (input_freq > 100))
    {
        scanf("%d", &input_freq);
    }
    printf("freq:%d \n", input_freq);

    xTaskCreate(signal_gen, "Signal generator", 2048, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(signal_acq, "Signal acquisition", 2048, NULL, tskIDLE_PRIORITY, NULL);
    //xTaskCreate(read_term, "Get user input", 1024, NULL, tskIDLE_PRIORITY, NULL);

}

void signal_gen(void *pvParameters) {
    const TickType_t xPeriod = pdMS_TO_TICKS(1000/input_freq);
    // Initialize the last wake time to the current time
    while(1){
            //ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
            blink_led();
            /* Toggle the LED state */
            s_led_state = !s_led_state;
            vTaskDelay(xPeriod);
    }
}
/* Task that */
void signal_acq(void *pvParameters) {
    uint32_t start;
    uint32_t end;
    uint32_t freq=0;
    while(1){
        if(gpio_get_level(GPIO_INPUT_IO_0) == 0){
            start = esp_timer_get_time();
            while(gpio_get_level(GPIO_INPUT_IO_0) == 0){
            }
            end = esp_timer_get_time();
            freq = 1000000 / (end - start); 
            ESP_LOGI(TAG, "Signal period = %lu\n", end - start);
            printf("Signal frequency = %lu Hz\n", freq);
        }
        vTaskDelay(1);
    }
}


