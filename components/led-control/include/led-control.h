
#ifndef _LED_CONTROL_H_
#define _LED_CONTROL_H_

#include <stdio.h>
#include "led_strip.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <math.h>
#include "sdkconfig.h"

#define LED_INTENSITY CONFIG_LED_INTENSITY
#define BLINK_GPIO CONFIG_BLINK_GPIO 

void blink_led(void);
void configure_led(void);

typedef struct colour {
   int red;
   int green;
   int blue;
} colour ;

enum colourStrings {
   red,
   blue,
   green,
   purple,
   off,
   white,
   cyan,
   orange,
   yellow,
};

colour getRGBVal(enum colourStrings);
void set_led(enum colourStrings target);


#endif /* _LED_CONTROL_H_ */