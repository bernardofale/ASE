# ASE - Project
Project developed for the ASE course
## Group members
| Nmec | Name | Github |
| :--: | :--- | :----- |
| 93331 | Bernardo Falé | [@bernardofale](https://github.com/bernardofale) |
| 93196 | Pedro Pereira | [@pedrocjdpereira](https://github.com/pedrocjdpereira) |

## Directory description
### distSensor
Source code for the ESP32 program to be flashed.
### webapp
Source code for Web App to be run on the host.

## Configurations

1. Connect your host pc to your mobile hotspot.

2. In a terminal, run the command:
`ifconfig`

3. Verify your assigned IP address in hotspot network.

4. In [File Name](distSensor/components/tcp_setup/tcp_setup.c) change the value HOST_IP_ADDR to your assigned IP address.

5. In [File Name](distSensor/components/wifi_setup/wifi_setup.c) change the values of EXAMPLE_ESP_WIFI_SSID and EXAMPLE_ESP_WIFI_PASS to the SSID and Password of the hotspot network.

## Usage

### In ESP-IDF Terminal

1. `cd distSensor/`

2. `idf.py set-target esp32`

3. `idf.py build`

4. `idf.py -p /dev/ttyUSB0 flash monitor`

### In separate terminal

5. `cd webapp/src/`

6. `python3 main.py`

7. Navigate to [Link Text](localhost:3000)

#