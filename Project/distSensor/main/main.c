#include <stdio.h>
#include "adc_setup.h"
#include "wifi_setup.h"
#include "tcp_setup.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_system.h"
#include "freertos/semphr.h"
#include "esp_adc/adc_continuous.h"
#include <string.h>
#include "esp_wifi.h"
#include <stdlib.h>
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_netif.h"
#include "cJSON.h"
#include "driver/gptimer.h"
#include "rom/ets_sys.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "esp_vfs_fat.h"
#include "nvs.h"
#include "cmd_system.h"
#include "cmd_wifi.h"
#include "cmd_nvs.h"
#include "lcdlib.h"
#include <driver/i2c.h>
#include "esp_sleep.h"

/* Ultra-sonic sensor GPIO definition */
#define TRIGGER_GPIO GPIO_NUM_25
#define ECHO_GPIO GPIO_NUM_26

/* LCD pins and address definitions */
#define LCD_ADDR 0x27
#define SDA_PIN  21
#define SCL_PIN  22
#define LCD_COLS 16
#define LCD_ROWS 2

/* Sampling related macros */
#define MAXIMUM_SAMPLING_RATE 70 /* in ms*/
#define MINIMUM_SAMPLING_RATE 1000 /* in ms*/
#define ADC_MAX 4095 /* Maximum ADC value */

/* Thread functions prototypes */
void distance( void *pvParameters );
void dashboard( void *pvParameters );
void display( void *pvParameters );
void console( void *pvParameters );
void sampling_rate( void *pvParameters );

/* ADC definitions */
#define READ_LEN 256
#if CONFIG_IDF_TARGET_ESP32
 static adc_channel_t channel[1] = {ADC_CHANNEL_6}; /* GPIO 34 */
 #endif
 adc_continuous_handle_t handle;
 uint8_t result[READ_LEN] = {0};

/* Semaphore definition */
SemaphoreHandle_t display_semaphore;
SemaphoreHandle_t mutex_sampling;

/* Timer definitions */
gptimer_handle_t distance_timer_handle;

static const char *TAG = "Motion Sensor";

/* Global distance and sampling rate variables */
static float dist = 0.0;
static float s_rate = 70.0;

void app_main(void)
{   

    /* ADC setup */
    TaskHandle_t s_task_handle = xTaskGetCurrentTaskHandle();
    setTaskHandle(s_task_handle);
    
    continuous_adc_init(channel, sizeof(channel) / sizeof(adc_channel_t), &handle, "ADC Setup");
    
    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = s_conv_done_cb,
    };
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(handle, &cbs, NULL));
    ESP_ERROR_CHECK(adc_continuous_start(handle));
    

    /* Semaphore creation */
    display_semaphore = xSemaphoreCreateBinary();
    mutex_sampling = xSemaphoreCreateMutex();
    
    if(display_semaphore == NULL){
        ESP_LOGI("Semaphore creation", "Error creating semaphore");
    }
    if(mutex_sampling == NULL){
        ESP_LOGI("Mutex_sampling creation", "Error creating mutex_sampling");
    }
    xSemaphoreGive(mutex_sampling);

    /* LCD setup */
    //LCD_init(LCD_ADDR, SDA_PIN, SCL_PIN, LCD_COLS, LCD_ROWS);

    /* Distance timer setup */
    distance_timer_handle = NULL;
    gptimer_config_t distance_timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1 * 1000 * 1000 //1us
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&distance_timer_config, &distance_timer_handle));

    /* GPIO definition */
    gpio_reset_pin(TRIGGER_GPIO);
    gpio_reset_pin(ECHO_GPIO);
    gpio_set_direction(TRIGGER_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(ECHO_GPIO, GPIO_MODE_INPUT);    
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    /* WiFi Setup */
    wifi_init_sta();
    
    /* Console Setup */
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    /* Prompt to be printed before each line. */
    repl_config.prompt = "distSensor>";
    repl_config.max_cmdline_length = 1024;

    /* Register commands */
    esp_console_register_help_command();
    register_system();

    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));

    ESP_ERROR_CHECK(esp_console_start_repl(repl));
    
    /* Task creation */
    xTaskCreate(&distance, "Distance Task", 4096, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(&dashboard, "Dashboard", 4096, NULL, tskIDLE_PRIORITY, NULL);
    //xTaskCreate(&display, "Display distance", 4096, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(&sampling_rate, "Sampling rate", 4096, NULL, tskIDLE_PRIORITY + 1, NULL);
}

void sampling_rate( void *pvParameters )
{
    ESP_LOGI("Sampling rate", "Start operations");
    uint32_t ret_num = 0;

    while(1){
        int ret;
        ret = adc_continuous_read(handle, result, READ_LEN, &ret_num, 0);
        adc_digi_output_data_t *p = (void*)&result[0];
        //printf("Potentiometer value: %d\n", p->type1.data);
        /* Sampling rate (ms) = (Potentiometer value / 4095) * (Maximum sampling rate - Minimum sampling rate) + Minimum sampling rate */
        xSemaphoreTake(mutex_sampling, portMAX_DELAY);
        s_rate = ((float)(ADC_MAX - p->type1.data) / ADC_MAX) * (MAXIMUM_SAMPLING_RATE - MINIMUM_SAMPLING_RATE) + MINIMUM_SAMPLING_RATE;;
        xSemaphoreGive(mutex_sampling);
        vTaskDelay(10);
    }
}

void distance( void *pvParameters ){
    ESP_LOGI("Distance", "Start operations");
    uint16_t echo_duration = 0;

    while(1)
    {   
        
        /* Sending pulse to activate sensor trigger */
        gpio_set_level(TRIGGER_GPIO, 1);
        ets_delay_us(10);
        gpio_set_level(TRIGGER_GPIO, 0);

        /* Wait until echo signals reception */
        int level = gpio_get_level(ECHO_GPIO);
        while (level == 0) level = gpio_get_level(ECHO_GPIO);

        /* -> Echo starts reception
           -> Start timer to count the duration of the echo 
        */
        ESP_ERROR_CHECK(gptimer_enable(distance_timer_handle));
        ESP_ERROR_CHECK(gptimer_start(distance_timer_handle));

        while (level == 1) level = gpio_get_level(ECHO_GPIO);

        /* Get the high precision timer in us */
        ESP_ERROR_CHECK(gptimer_get_raw_count(distance_timer_handle, &echo_duration));

        /* Calculate the distance based on the duration of the echo */
        dist = (float)echo_duration * 0.034 / 2.0;
        
        /* Stop and reset the timer */
        ESP_ERROR_CHECK(gptimer_stop(distance_timer_handle));
        ESP_ERROR_CHECK(gptimer_set_raw_count(distance_timer_handle, 0));
        ESP_ERROR_CHECK(gptimer_disable(distance_timer_handle));
        xSemaphoreGive(display_semaphore);

        xSemaphoreTake(mutex_sampling, portMAX_DELAY);
        vTaskDelay(((int) s_rate) / portTICK_PERIOD_MS);
        xSemaphoreGive(mutex_sampling);
    }
}

void display( void *pvParameters )
{
    ESP_LOGI("Display", "Start operations");
    char buffer[20];
    char buffer2[20];
    while(1){
        xSemaphoreTake(display_semaphore, portMAX_DELAY);
        snprintf(buffer, sizeof(buffer), "%.2f", dist);
        xSemaphoreTake(mutex_sampling, portMAX_DELAY);
        snprintf(buffer2, sizeof(buffer2), "%.2f", s_rate);
        //printf("sampling rate: %f ms\n", s_rate);
        xSemaphoreGive(mutex_sampling);
        strcat(buffer, " Cm");
        strcat(buffer2, " Ms");
        LCD_home();
        LCD_clearScreen();
        LCD_writeStr(buffer);
        LCD_setCursor(0, 1);
        LCD_writeStr(buffer2);
        vTaskDelay(300 / portTICK_PERIOD_MS);
    }
}

void dashboard( void *pvParameters )
{
    ESP_LOGI("Dashboard", "Start operations");
    while(1){
        char* s = (char*) pvPortMalloc(50 * sizeof(char));
        xSemaphoreTake(mutex_sampling, portMAX_DELAY);
        snprintf(s, 50, "{\"distance\": \"%.2f\", \"samplerate\": \"%.2f\"}", dist, s_rate);
        xSemaphoreGive(mutex_sampling);
        tcp_client("Dashboard", s);
        vPortFree(s);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
