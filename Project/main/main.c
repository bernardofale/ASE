#include <stdio.h>
#include "fft.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_system.h"
#include "freertos/semphr.h"


/* Thread functions prototypes */
void vibe_acq( void *pvParameters );
void calc_fft( void *pvParameters );
void print_seg( void *pvParameters );
void dashboard( void *pvParameters );

/* Semaphore definition */
SemaphoreHandle_t acq_to_fft;

void app_main(void)
{

    

    /* Semaphore creation */
    acq_to_fft = xSemaphoreCreateBinary();
    if(temp_to_mean == NULL)
    {
        ESP_LOGI("Semaphore", "Semaphore not created");
    }

    xTaskCreate(&vibe_acq, "Vibration Acquisition", 2048, NULL, tskIDLE_PRIORITY + 4, NULL);
    xTaskCreate(&calc_fft, "Calculate FFT", 2048, NULL, tskIDLE_PRIORITY + 3, NULL);
    xTaskCreate(&print_seg, "Print to 7 segments", 2048, NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(&dashboard, "Dashboard", 2048, NULL, tskIDLE_PRIORITY + 4, NULL);
}

void vibe_acq( void *pvParameters )
{
    while(1)
    {
        ESP_LOGI("Vibration Acquisition");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void calc_fft( void *pvParameters )
{
    while(1)
    {
        ESP_LOGI("Calculate FFT");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void print_seg( void *pvParameters )
{
    while(1)
    {
        ESP_LOGI("Print to 7 segments");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void dashboard( void *pvParameters )
{
    while(1)
    {
        ESP_LOGI("Dashboard");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}