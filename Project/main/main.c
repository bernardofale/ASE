#include <stdio.h>
#include "fft.h"
#include "adc_setup.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_system.h"
#include "freertos/semphr.h"
#include "esp_adc/adc_continuous.h"

#define READ_LEN 256

/* Thread functions prototypes */
void vibe_acq( void *pvParameters );
void calc_fft( void *pvParameters );
void print_seg( void *pvParameters );
void dashboard( void *pvParameters );

/* Semaphore definition */
SemaphoreHandle_t acq_to_fft;

/* ADC definitions */
#if CONFIG_IDF_TARGET_ESP32
static adc_channel_t channel[1] = {ADC_CHANNEL_7}; /* GPIO 35 */
#endif
adc_continuous_handle_t handle = NULL;
uint8_t result[READ_LEN] = {0};


static const char *TAG = "Guitar Tuner";

void app_main(void)
{
    

    /* ADC Setup */
    esp_err_t ret;
    memset(result, 0xcc, READ_LEN);
    TaskHandle_t s_task_handle = xTaskGetCurrentTaskHandle();
    setTaskHandle(s_task_handle);

    continuous_adc_init(channel, sizeof(channel) / sizeof(adc_channel_t), &handle);

    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = s_conv_done_cb,
    };
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(handle, &cbs, NULL));
    ESP_ERROR_CHECK(adc_continuous_start(handle));

    /* Semaphore creation */
    acq_to_fft = xSemaphoreCreateBinary();
    if(acq_to_fft == NULL)
    {
        ESP_LOGI("Semaphore", "Semaphore not created");
    }

    /* Task creation */
    xTaskCreate(&vibe_acq, "Vibration Acquisition", 4096, NULL, tskIDLE_PRIORITY + 4, NULL);
    xTaskCreate(&calc_fft, "Calculate FFT", 2048, NULL, tskIDLE_PRIORITY + 3, NULL);
    xTaskCreate(&print_seg, "Print to 7 segments", 2048, NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(&dashboard, "Dashboard", 2048, NULL, tskIDLE_PRIORITY + 4, NULL);
}

void vibe_acq( void *pvParameters )
{
    //ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    uint16_t ret;
    uint32_t ret_num = 0;

    while(1)
    {
        ESP_LOGI("Vibration Acquisition", "Reading Value");
        ret = adc_continuous_read(handle, result, READ_LEN, &ret_num, 0);
        if(ret == ESP_OK){
            //ESP_LOGI("TASK", "ret is %x, ret_num is %"PRIu32, ret, ret_num);
            for (int i = 0; i < ret_num; i += SOC_ADC_DIGI_RESULT_BYTES) {
                    adc_digi_output_data_t *p = (void*)&result[i];
                #if CONFIG_IDF_TARGET_ESP32
                if(i == 10){
                            ESP_LOGI(TAG, "Unit: %d, Channel: %d, Value: %d", 1, p->type1.channel, p->type1.data);
                }
                #endif
                }
        }
        vTaskDelay(1);
    }
}

void calc_fft( void *pvParameters )
{
    while(1)
    {
        ESP_LOGI("Calculate FFT", "Start operations");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void print_seg( void *pvParameters )
{
    while(1)
    {
        ESP_LOGI("Print to 7 segments", "Start operations");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void dashboard( void *pvParameters )
{
    while(1)
    {
        ESP_LOGI("Dashboard", "Start operations");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}