#include <stdio.h>
#include "adc_setup.h"
#include "fft.h"
#include "wifi_setup.h"
#include "tcp_setup.h"
#include "display7seg.h"
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

#define TRIGGER_GPIO GPIO_NUM_25
#define ECHO_GPIO GPIO_NUM_26

/* Thread functions prototypes */
void distance( void *pvParameters );
void dashboard( void *pvParameters );
void console( void *pvParameters );

/* Timer definitions */
gptimer_handle_t distance_timer_handle;

static const char *TAG = "Motion Sensor";

static int dist = 100;

void app_main(void)
{

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
    
    /* WiFi Setup */
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init_sta();
    
    /* Console Setup */
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    /* Prompt to be printed before each line.
     * This can be customized, made dynamic, etc.
     */
    repl_config.prompt = "distSensor>";
    repl_config.max_cmdline_length = 1024;

    /* Register commands */
    esp_console_register_help_command();
    register_system();

    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));

    ESP_ERROR_CHECK(esp_console_start_repl(repl));
    
    /* Task creation */
    xTaskCreate(&distance, "Distance Task", 4096, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(&dashboard, "Dashboard", 4096, NULL, tskIDLE_PRIORITY + 1, NULL);
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
        float distance = (float)echo_duration * 0.034 / 2.0;
        printf("Distance: %.2f cm\n", distance);
        
        /* Stop and reset the timer */
        ESP_ERROR_CHECK(gptimer_stop(distance_timer_handle));
        ESP_ERROR_CHECK(gptimer_set_raw_count(distance_timer_handle, 0));
        ESP_ERROR_CHECK(gptimer_disable(distance_timer_handle));

        vTaskDelay(70 / portTICK_PERIOD_MS);
    }
}

void dashboard( void *pvParameters )
{
    ESP_LOGI("Dashboard", "Start operations");
    int i = 0;
    while(1){
        if(i == 7) i = 0;
        char* s = (char*) pvPortMalloc(40 * sizeof(char));
        snprintf(s, 40, "{\"distance\": \"%d\", \"motorOn\": \"%s\"}", i * 100, (i*100 < 400) ? "false" : "true");
        tcp_client("Dashboard", s);
        vPortFree(s);
        i++;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}