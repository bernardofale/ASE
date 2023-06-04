#include "tcp_setup.h"

#define HOST_IP_ADDR "192.168.43.12"
#define PORT 5000

static int tcp_socket = -1; // Global variable to store the socket

static char* host_ip;

int tcp_innit(const char *TAG)
{
    host_ip = HOST_IP_ADDR;
    int addr_family = 0;
    int ip_protocol = 0;

    while (1) {
        struct sockaddr_in dest_addr;
        inet_pton(AF_INET, host_ip, &dest_addr.sin_addr);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;

        int sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            close(sock);
            vTaskDelay(3000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "Socket created, connecting to %s:%d", host_ip, PORT);

        int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
            close(sock);
            vTaskDelay(3000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "Successfully connected");
        return sock;
    }
}

void tcp_client(const char *TAG, const char *payload){
    while (tcp_socket == -1) {
        tcp_socket = tcp_innit(TAG); // Call tcp_init only if the socket connection is not initialized
        if (tcp_socket == -1) {
            ESP_LOGE(TAG, "Failed to establish the connection");
        }
    }

    char rx_buffer[128];
    
    int err = send(tcp_socket, payload, strlen(payload), 0);
    if (err < 0) {
        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
        ESP_LOGE(TAG, "Shutting down socket...");
        shutdown(tcp_socket, 0);
        close(tcp_socket);
        tcp_socket = -1;
    }

    int len = recv(tcp_socket, rx_buffer, sizeof(rx_buffer) - 1, 0);
    // Error occurred during receiving
    if (len < 0) {
        ESP_LOGE(TAG, "recv failed: errno %d", errno);
        ESP_LOGE(TAG, "Shutting down socket...");
        shutdown(tcp_socket, 0);
        close(tcp_socket);
        tcp_socket = -1;
    }
    // Data received
    else {
        rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
        ESP_LOGI(TAG, "Received %d bytes from %s:", len, host_ip);
        ESP_LOGI(TAG, "%s", rx_buffer);
    }
}