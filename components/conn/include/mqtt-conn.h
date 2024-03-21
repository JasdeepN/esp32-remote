#ifndef _MQTT_CONN_H_
#define _MQTT_CONN_H_
// sntp example
// #include <string.h>
#include "conn.h"
#include <time.h>
#include <sys/time.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "esp_netif_sntp.h"
#include "lwip/ip_addr.h"
#include "esp_sntp.h"
#include "sdkconfig.h"
#include "time-sync.h"

// MQTT Publishing:
#define MQTT_BROKER_URL CONFIG_MQTT_BROKER_URL  
#define MQTT_BROKER_PORT CONFIG_MQTT_BROKER_PORT 
#define MQTT_QOS 2 
#define MQTT_RETAIN 3 
#define MQTT_MAIN_TOPIC CONFIG_MQTT_TOPIC
#define MQTT_SUB_TOPIC CONFIG_MQTT_SUB

#define MQTT_USER CONFIG_MQTT_USER
#define MQTT_PASS CONFIG_MQTT_PASSWORD

#define REPORTING_FREQUENCY_IN_MINUTES 1

// time out period (in seconds) to complete the MQTT publishing
#define MQTT_PUBLISHING_TIMEOUT_PERIOD 30 

void MQTT_publish_a_reading(const char *subtopic, float value);
void MQTT_publish_all_readings();
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
void publish_readings_via_MQTT();
void send_mqtt();


#endif // _MQTT_CONN_H_