# _ESP32-c6 (mostly) Wireless Remote Sensor_

This Project uses https://github.com/roblatour/WeatherStation as a base: 

    Copyright Rob Latour, 2023
    License MIT
    open source project: https://github.com/roblatour/WeatherStation

Mosquitto Docker setup
    https://github.com/sukesh-ak/setup-mosquitto-with-docker?tab=readme-ov-file

## ESP32-C6

ESP32-C6 is Espressif’s first Wi-Fi 6 SoC integrating 2.4 GHz Wi-Fi 6, Bluetooth 5 (LE) and the 802.15.4 protocol. ESP32-C6 achieves an industry-leading RF performance, with reliable security features and multiple memory resources for IoT products. It consists of a high-performance (HP) 32-bit RISC-V processor, which can be clocked up to 160 MHz, and a low-power (LP) 32-bit RISC-V processor, which can be clocked up to 20 MHz. It has a 320KB ROM, a 512KB SRAM, and works with external flash. It comes with 30 (QFN40) or 22 (QFN32) programmable GPIOs, with support for SPI, UART, I2C, I2S, RMT, TWAI, PWM, SDIO, Motor Control PWM. It also packs a 12-bit ADC and a temperature sensor.

## main folder contents

The project **main** contains one source file in C language [main.c](main/main.c). The file is located in folder [main](main).

ESP-IDF projects are built using CMake. The project build configuration is contained in `CMakeLists.txt`
files that provide set of directives and instructions describing the project's source files and targets
(executable, library, or both). 

Below is short explanation of remaining files in the project folder.

```
├── CMakeLists.txt
├── main
│   ├── CMakeLists.txt
│   ├── main.c
|   ├── idf_component.yml
|   ├── general_user_settings.h
|   └── secret_user_settings.h
├── components
|   ├── bme680
|   ├── cmd_system
|   ├── esp_idf_lib_helpers
|   ├── i2cdev
|   └── iperf
├── managed_components
|   ├── espressif__console_cmd_wifi
|   ├── espressif__console_simple_init
|   └── espressif__led_strip
└── README.md                  This is the file you are currently reading
```
Additionally, the sample project contains Makefile and component.mk files, used for the legacy Make based build system. 
They are not used or needed when building with CMake and idf.py.
