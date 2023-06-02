/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

// --------------------------------------------------
// WIFI IMPORTS
// --------------------------------------------------

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "wifi_setup.h"

// --------------------------------------------------
// TCP IMPORTS
// --------------------------------------------------

#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "sdkconfig.h"
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>            // struct addrinfo
#include <arpa/inet.h>
#include "esp_log.h"
#if defined(CONFIG_EXAMPLE_SOCKET_IP_INPUT_STDIN)
#include "addr_from_stdin.h"
#endif

#include "tcp_setup.h"

static const char *TAG = "WIFI APP";

// --------------------------------------------------
// MAIN FUNCTION
// --------------------------------------------------

void app_main(void)
{
    // --------------------------------------------------
    // Establish Wifi Connection
    // --------------------------------------------------

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta(TAG);

    // --------------------------------------------------
    // Establish TCP socket connection with the server
    // --------------------------------------------------

    tcp_client(TAG);
}
