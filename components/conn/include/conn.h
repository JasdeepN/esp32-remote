#ifndef _CONN_H_
#define _CONN_H_

#include "led-control.h"
#include "secret_user_settings.h"
#include "general_user_settings.h"

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

// WIFI
#define ITWT_TRIGGER_ENABLED 1 // 0 = FALSE, 1 = TRUE
#define ITWT_ANNOUNCED 1       // 0 = FALSE, 1 = TRUE
#define ITWT_MIN_WAKE_DURATION 255

#define WIFI_LISTEN_INTERVAL 100

// TPL5100 // TODO: Get this out too (not using TPL5100)
#define HIGH 1
#define LOW 0


// Global variables
// TODO: get this shit out of here
extern bool BME680_readings_are_reasonable;
extern float temperature;
extern float humidity;
extern float pressure;

void turn_on_Wifi(void);
void restart_after_this_many_seconds(int s);
#endif /* _CONN_H_ */
