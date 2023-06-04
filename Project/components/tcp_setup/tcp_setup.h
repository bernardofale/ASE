#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
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

int tcp_innit();

void tcp_client(const char* payload);