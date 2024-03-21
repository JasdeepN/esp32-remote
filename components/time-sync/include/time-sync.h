
#ifndef _TIME_SYNC_H_
#define _TIME_SYNC_H_

#include "conn.h"
#include "sdkconfig.h"

// sntp example
// #include <string.h>
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
#include "led-control.h"


char* get_time(void);
void obtain_time(void);
void init_ntp(void);
void print_servers(void);
void time_sync_notification_cb(struct timeval *tv);


void func(void);

#endif // _TIME_SYNC_H_