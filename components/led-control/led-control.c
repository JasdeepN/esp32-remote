#include "led-control.h"

static uint8_t s_led_state = 0;
static led_strip_handle_t led_strip;
// double INTENSITY = 0.10;

colour getRGBVal(enum colourStrings args) {
    switch (args){
        case off:
            return (colour){ 0, 0, 0 };
            break;
        case purple:
            return (colour){ 128, 0, 128 };
            break;
        case red:
            return (colour){ 255, 0, 0 };
            break;
        case green:
            return (colour){ 0, 255, 0 };
            break;
        case blue:
            return (colour){ 0, 0, 255 };
            break;
        case cyan:
            return (colour){ 0, 255, 255 };
            break;
        case white:
            return (colour){ 255, 255, 255 };
            break;
        case orange:
            return (colour){ 255, 69, 0 };
            break;      
        case yellow:
            return (colour){ 255, 215, 0 };
            break;                                                             
    }
    return (colour){ 0, 0, 0 };
}

// colour status[7] = {
//    { 255, 255, 0   }, // yellow 0
//    { 0,   255, 0   }, // GREEN  1
//    { 255, 0,   0   }, // RED    2
//    { 128, 0,   128 }, // purple 3
//    { 0,   0,   255 }, // blue   4
//    { 0,   0,   0   }, // off    5
//    { 128, 128, 128 }, // white 50% power
// };

void blink_led(void)
{
    /* If the addressable LED is enabled */
    if (s_led_state) {
        /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
        led_strip_set_pixel(led_strip, 0, 16, 16, 16);
        /* Refresh the strip to send data */
        led_strip_refresh(led_strip);
    } else {
        /* Set all LED off to clear all pixels */
        led_strip_clear(led_strip);
    }
}

void configure_led(void)
{
    ESP_LOGI("Startup", "Configure LED Start");
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 1, // at least one LED on board
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
    ESP_LOGI("Startup", "Configure LED Done.");
}

void set_led(enum colourStrings target)
{
    colour status = getRGBVal(target);
    double intensity = (LED_INTENSITY/100.0);
    ESP_LOGI("Set LED", "Set LED to (%f,%f,%f)", ceil(status.red*intensity), ceil(status.green*intensity), ceil(status.blue*intensity));

    led_strip_set_pixel(led_strip, 0, ceil(status.red*intensity), ceil(status.green*intensity), ceil(status.blue*intensity));
    /* Refresh the strip to send data */
    led_strip_refresh(led_strip);
   
}