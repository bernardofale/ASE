idf_component_register(SRCS "tcp_setup.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash
                    REQUIRES esp_netif)