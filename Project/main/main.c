#include <stdio.h>
#include "adc_setup.h"
#include "fft.h"
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
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sys.h"

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

static int temp = 0;

// WIFI DEFINITIONS
/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      "Redmi"
#define EXAMPLE_ESP_WIFI_PASS      "abcd1234"
#define EXAMPLE_ESP_MAXIMUM_RETRY  100

#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0;

// WIFI AUXILIARY FUNCTIONS
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI("Dashboard", "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI("Dashboard","connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI("Dashboard", "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (pasword len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
	     * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI("Dashboard", "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI("Dashboard", "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI("Dashboard", "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE("Dashboard", "UNEXPECTED EVENT");
    }
}

void app_main(void)
{
    /* ADC Setup */
    memset(result, 0xcc, READ_LEN);
    TaskHandle_t s_task_handle = xTaskGetCurrentTaskHandle();
    setTaskHandle(s_task_handle);

    continuous_adc_init(channel, sizeof(channel) / sizeof(adc_channel_t), &handle, "Vibration Acquisition");

    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = s_conv_done_cb,
    };
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(handle, &cbs, NULL));
    ESP_ERROR_CHECK(adc_continuous_start(handle));

    /* Display7Seg Setup */
    display7seg_setup();

    /* WiFi Setup */
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI("Dashboard", "ESP_WIFI_MODE_STA");
    wifi_init_sta();

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

    int cnt = 0;
    int med = 0;
    while(1)
    {
        med = 0;
        ESP_LOGI("Vibration Acquisition", "Reading Value");
        ret = adc_continuous_read(handle, result, READ_LEN, &ret_num, 0);
        if(ret == ESP_OK){
            //ESP_LOGI("TASK", "ret is %x, ret_num is %"PRIu32, ret, ret_num);
            for (int i = 0; i < ret_num; i += SOC_ADC_DIGI_RESULT_BYTES) {
                adc_digi_output_data_t *p = (void*)&result[i];
                med += p->type1.data;
            }
            temp = (int) med*SOC_ADC_DIGI_RESULT_BYTES/ret_num;
            if(cnt == 4){
                cnt = -1;
                ESP_LOGI("Vibration Acquisition", "Unit: %d, Channel: %d, Value: %d", 1, 7, temp);
            }
        }
        cnt++;
        vTaskDelay(100 / portTICK_PERIOD_MS);
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
    ESP_LOGI("Print to 7 segments", "Start operations");
    while(1) {
        display(temp%7);
        vTaskDelay(1000 / portTICK_PERIOD_MS); 
    }
}

void dashboard( void *pvParameters )
{
    ESP_LOGI("Dashboard", "Start operations");
    const char chars[] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G'
    };
    int i = 0;
    char* s = (char*)malloc(40 * sizeof(char));
    while(1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        
        if(i == 7) i = 0;
        snprintf(s, 40, "{\"frequency\": \"%dHz\", \"note\": \"%c\"}", temp, chars[temp%7]);
        tcp_client("Dashboard", s);
        i++;
    }
}