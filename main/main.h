#ifndef _MAIN_H_
#define _MAIN_H_

#include "sdkconfig.h"
#include "general_user_settings.h"
#include "conn.h"
#include "led-control.h"
#include "time-sync.h"



void test(void);

void startup(void);
void startup_validations_and_displays();
void initalize_non_volatile_storage();
void connect_to_WiFi();

static void initialize_power_management();


static void wait(int seconds);


/* Variable holding number of times ESP32 restarted since first boot.
 * It is placed into RTC memory using RTC_DATA_ATTR and
 * maintains its value when ESP32 wakes from deep sleep.
 */
RTC_DATA_ATTR static int boot_count = 0;




// BME680 sensor
#define HTTP_RESPONSE_BUFFER_SIZE 1024
#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048

// Used to power up and down the BME680 sensor
#define POWER_ON 1
#define POWER_OFF 0





#endif