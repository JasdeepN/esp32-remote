#ifndef _CONN_H_
#define _CONN_H_
/*
MIT License

Copyright (c) 2023 Rob Latour

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


// HEAVILY MODIFIED FROM ABOVE ..

// Sleep approach for reducing power consumption
#define USE_AUTOMATIC_SLEEP_APPROACH 2 // set to 0 to use deep sleep
                                                             // set to 1 to use automatic light sleep; also EDF-ISP: F1 -> SDK ESP-IDF:SDK Configuration editor (menuconfig) -> Component config -> FreeRTOS -> Tickless Idle (must be checked)
                                                             // set to 2 to use manual light sleep; also EDF-ISP: F1 -> SDK ESP-IDF:SDK Configuration editor (menuconfig) -> Component config -> FreeRTOS -> Tickless Idle (must be unchecked)
                                                             // set to 3 to use a TPL5100 board; also EDF-ISP: F1 -> SDK ESP-IDF:SDK Configuration editor (menuconfig) -> Component config -> FreeRTOS -> Tickless Idle (must be unchecked)
                                                             // for more information, please see: https://github.com/roblatour/WeatherStation

#include "led-control.h"

#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include <esp_sntp.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_pm.h"
#include "esp_sleep.h"
#include "esp_timer.h"

#include "driver/gpio.h"

#include "nvs_flash.h"

// #include <bme680.h>

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/err.h"
#include "lwip/api.h"

#include "mqtt_client.h"

#include "esp_http_client.h"

#include <cJSON.h>

#include <sys/param.h>
#include <stdlib.h>
#include <ctype.h>
#include "esp_netif.h"
#include "esp_tls.h"

#include "cmd_system.h"
#include "wifi_cmd.h"
#include "esp_wifi_he.h"

#include "mqtt-conn.h"

enum Wifi_status
{
    WIFI_CHECKING = 0,
    WIFI_UP = 1,
    WIFI_DOWN = 2
};


extern enum Wifi_status WiFi_current_status;

extern bool WiFi_is_connected;
extern bool WiFi6_TWT_setup_successfully;
extern bool MQTT_is_connected;
extern bool light_sleep_enabled;

extern int MQTT_published_messages;
extern bool MQTT_publishing_in_progress;
extern bool MQTT_unknown_error;

extern bool going_to_sleep;

extern bool ignore_disconnect_event;

extern esp_mqtt_client_handle_t MQTT_client;

extern int CONNECTED_BIT;
extern int DISCONNECTED_BIT;
extern esp_netif_t *netif_sta;
extern EventGroupHandle_t wifi_event_group;

extern int64_t cycle_start_time;

extern esp_pm_config_t power_management_disabled;
extern esp_pm_config_t power_management_enabled;

// WIFI required configs
#define ITWT_TRIGGER_ENABLED 1 // 0 = FALSE, 1 = TRUE
#define ITWT_ANNOUNCED 1       // 0 = FALSE, 1 = TRUE
#define ITWT_MIN_WAKE_DURATION 255
#define WIFI_LISTEN_INTERVAL 100

// WIFI SSID and PASS
#define WIFI_SSID CONFIG_ESP_WIFI_SSID
#define WIFI_PASS CONFIG_ESP_WIFI_PASSWORD

// Wi-Fi:
// Your country code, for more information please see https://wiki.recalbox.com/en/tutorials/network/wifi/wifi-country-code 
#define WIFI_COUNTRY_CODE "CA"

// time out period (in seconds) to connect to Wi-Fi
#define WIFI_CONNECT_TIMEOUT_PERIOD 10

// if WI-FI cannot connect within the time out period, then the ESP32 will go into deep sleep for this many seconds
#define DEEP_SLEEP_PERIOD_WHEN_WIFI_CANNOT_CONNECT 15

// TPL5100 // TODO: Get this out too (not using TPL5100)
#define HIGH 1
#define LOW 0

// TODO: Not being used I think.
// Target Wake Time mantissa and exponent, for more information please see https://github.com/roblatour/TWTWakeDurationCalculator 
#define ITWT_WAKE_INVL_EXPN 14    // for 10 seconds use 8;     for 15 minutes use 14
#define ITWT_WAKE_INVL_MANT 54932 // for 10 seconds use 39062; for 15 minutes use 54932


// Global variables
// TODO: get this shit out of here OOOOR ifndef env ESP gets these vars
extern bool BME680_readings_are_reasonable;
extern float temperature;
extern float humidity;
extern float pressure;

void turn_on_Wifi(void);
void restart_after_this_many_seconds(int s);
#endif /* _CONN_H_ */