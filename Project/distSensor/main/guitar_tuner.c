/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_adc/adc_continuous.h"
#include "adc_setup.h"
#include "fft.h"

#define EXAMPLE_READ_LEN   256
#define GET_UNIT(x)        ((x>>3) & 0x1)

#if CONFIG_IDF_TARGET_ESP32
#define ADC_CONV_MODE       ADC_CONV_SINGLE_UNIT_1  //ESP32 only supports ADC1 DMA mode
#define ADC_OUTPUT_TYPE     ADC_DIGI_OUTPUT_FORMAT_TYPE1
#endif

#if CONFIG_IDF_TARGET_ESP32
static adc_channel_t channel[1] = {ADC_CHANNEL_7};
#endif


static const char *TAG = "EXAMPLE";

void app_main(void)
{
    esp_err_t ret;
    uint32_t ret_num = 0;
    uint8_t result[EXAMPLE_READ_LEN] = {0};
    memset(result, 0xcc, EXAMPLE_READ_LEN);

    s_task_handle = xTaskGetCurrentTaskHandle();

    adc_continuous_handle_t handle = NULL;
    continuous_adc_init(channel, sizeof(channel) / sizeof(adc_channel_t), &handle);
    //fft_config_t *real_fft_plan = fft_init(NFFT, FFT_REAL, FFT_FORWARD, NULL, NULL);

    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = s_conv_done_cb,
    };
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(handle, &cbs, NULL));
    ESP_ERROR_CHECK(adc_continuous_start(handle));

    while(1) {

        /**
         * This is to show you the way to use the ADC continuous mode driver event callback.
         * This `ulTaskNotifyTake` will block when the data processing in the task is fast.
         * However in this example, the data processing (print) is slow, so you barely block here.
         *
         * Without using this event callback (to notify this task), you can still just call
         * `adc_continuous_read()` here in a loop, with/without a certain block timeout.
         */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        while (1) {
            ret = adc_continuous_read(handle, result, EXAMPLE_READ_LEN, &ret_num, 0);
            if (ret == ESP_OK) {
                ESP_LOGI("TASK", "ret is %x, ret_num is %"PRIu32, ret, ret_num);
                for (int i = 0; i < ret_num; i += SOC_ADC_DIGI_RESULT_BYTES) {
                    adc_digi_output_data_t *p = (void*)&result[i];
                    real_fft_plan->input[k] = (float)p->type1.data;
        #if CONFIG_IDF_TARGET_ESP32
                    ESP_LOGI(TAG, "Unit: %d, Channel: %d, Value: %x", 1, p->type1.channel, p->type1.data);
        #endif
                }
                // Execute transformation
                fft_execute(real_fft_plan);

                // Now do something with the output
                printf("DC component : %f\n", real_fft_plan->output[0]);  // DC is at [0]
                for (k = 1 ; k < real_fft_plan->size / 2 ; k++)
                printf("%d-th freq : %f+j%f\n", k, real_fft_plan->output[2*k], real_fft_plan->output[2*k+1]);
                printf("Middle component : %f\n", real_fft_plan->output[1]);  // N/2 is real and stored at [1]

                // Don't forget to clean up at the end to free all the memory that was allocated
                fft_destroy(real_fft_plan)
                /**
                 * Because printing is slow, so every time you call `ulTaskNotifyTake`, it will immediately return.
                 * To avoid a task watchdog timeout, add a delay here. When you replace the way you process the data,
                 * usually you don't need this delay (as this task will block for a while).
                 */
                vTaskDelay(1);
            } else if (ret == ESP_ERR_TIMEOUT) {
                //We try to read `EXAMPLE_READ_LEN` until API returns timeout, which means there's no available data
                break;
            }
        }
    }

    ESP_ERROR_CHECK(adc_continuous_stop(handle));
    ESP_ERROR_CHECK(adc_continuous_deinit(handle));
}
