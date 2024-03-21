// using modified code from:
   // Copyright Rob Latour, 2023
   // License MIT
   // open source project: https://github.com/roblatour/WeatherStation

   // Running on an esp32-c6 for Wifi 6 (802.11ax) using either:
   //  - deep sleep, or
   //  - automatic light sleep with WiFi 6
   //  to reduce power consumption.
   //
   // For more information please see the general_user_settings.h file

#include "main.h"

static char* TAG = "MAIN";

static void initialize_power_management()
{

    // power management is only needed when the program will use automatic light sleep

    if (GENERAL_USER_SETTINGS_USE_AUTOMATIC_SLEEP_APPROACH == 1)
    {

        // get the current power management configuration and save it as a baseline for when power save mode is disabled
        esp_pm_get_configuration(&power_management_disabled);

        // set the power management configuration for when the power save mode is enabled
#if CONFIG_PM_ENABLE
        esp_pm_config_t power_management_when_enabled = {
            .max_freq_mhz = 160, // ref: the esp32-c6 datasheet https://www.espressif.com/sites/default/files/documentation/esp32-c6-wroom-1_wroom-1u_datasheet_en.pdf
            .min_freq_mhz = 10,  // ref: Espressive's itwt example: https://github.com/espressif/esp-idf/tree/903af13e847cd301e476d8b16b4ee1c21b30b5c6/examples/wifi/itwt
#if CONFIG_FREERTOS_USE_TICKLESS_IDLE
            .light_sleep_enable = true
#endif
        };
        power_management_enabled = power_management_when_enabled;
#endif
    }
};




void startup(void){
   ++boot_count;
   ESP_LOGI(TAG, "Boot count: %d", boot_count);
   configure_led();

   set_led(orange);

   startup_validations_and_displays();

   initalize_non_volatile_storage();

   initialize_power_management();

   // initialize_the_external_switch();

   connect_to_WiFi();
   init_ntp();
   ESP_LOGI("Startup", "Complete");
   set_led(cyan);

};






// void get_bme680_readings()
// {

//     // Gets temperature, pressure and humidity from the BME680.
//     // Do not get gas_resistance.

//     static bool doOnce = true;

//     static bme680_t sensor;

//     bme680_values_float_t values;

//     temperature = 0;
//     humidity = 0;
//     pressure = 0;

//     BME680_readings_are_reasonable = false;

//     // power up the BME680 sensor
//     // one time setup for the BME680 sensor power pin
//     if (doOnce)
//     {
//         esp_rom_gpio_pad_select_gpio(GENERAL_USER_SETTINGS_POWER_SENSOR_CONTROLLER_PIN);
//         gpio_reset_pin(GENERAL_USER_SETTINGS_POWER_SENSOR_CONTROLLER_PIN);
//         gpio_set_direction(GENERAL_USER_SETTINGS_POWER_SENSOR_CONTROLLER_PIN, GPIO_MODE_OUTPUT);
//         doOnce = false;
//     };

//     gpio_set_level(GENERAL_USER_SETTINGS_POWER_SENSOR_CONTROLLER_PIN, POWER_ON);
//     ESP_LOGI("BME680", "BME680 powered on");

//     // wait for the sensor to fully power up
//     vTaskDelay(25 / portTICK_PERIOD_MS);

//     ESP_LOGI("BME680", "taking BME680 readings");

//     // initialize the sensor
//     ESP_ERROR_CHECK(i2cdev_init());

//     memset(&sensor, 0, sizeof(bme680_t));
//     ESP_ERROR_CHECK(bme680_init_desc(&sensor, GENERAL_USER_SETTINGS_BME680_I2C_ADDR, GENERAL_USER_SETTINGS_PORT, GENERAL_USER_SETTINGS_I2C_SDA, GENERAL_USER_SETTINGS_I2C_SCL));
//     // wait for the sensor to power up

//     ESP_ERROR_CHECK(bme680_init_sensor(&sensor));

//     // turn off reporting for gas_resistance
//     bme680_use_heater_profile(&sensor, BME680_HEATER_NOT_USED);

//     // set the oversampling rate to 16x for temperature, pressure and humidity.
//     bme680_set_oversampling_rates(&sensor, BME680_OSR_16X, BME680_OSR_16X, BME680_OSR_16X);

//     // Set the IIR filter size
//     // The purpose of the IIR filter is to remove noise and fluctuations from the sensor data, which can improve the accuracy and stability of the sensor readings.
//     // The filter size determines how much filtering is applied to the sensor data, with larger filter sizes resulting in smoother but slower sensor readings.
//     // The bme680_set_filter_size() function may be set to one of several predefined values, depending on the level of filtering required.
//     // The available filter sizes range from 0 (no filtering) to 127 (maximum filtering).
//     // By selecting an appropriate filter size, you can balance the trade-off between sensor response time and accuracy, depending on the specific needs of your application.
//     bme680_set_filter_size(&sensor, BME680_IIR_SIZE_127);

//     // Set ambient temperature n/a
//     // bme680_set_ambient_temperature(&sensor, 20);

//     // get the delay time (duration) required to get a set of readings
//     uint32_t duration;
//     bme680_get_measurement_duration(&sensor, &duration);

//     // for some reason the first measurement is always wrong
//     // so take a throw away measurement
//     // this measurement will not counted in the attempts counter below
//     if (bme680_force_measurement(&sensor) == ESP_OK)
//     {
//         vTaskDelay(duration / portTICK_PERIOD_MS);
//         if (bme680_get_results_float(&sensor, &values) == ESP_OK)
//         {
//             // ESP_LOGI("BME680", "throw away measurement taken");
//         }
//     };

//     int attempts = 0;
//     int max_attempts = 10;
//     // take up to max_attempts measurements until the readings are valid

//     while (!BME680_readings_are_reasonable && (attempts++ < max_attempts))
//     {

//         if (bme680_force_measurement(&sensor) == ESP_OK)
//         {
//             vTaskDelay(duration / portTICK_PERIOD_MS); // wait until measurement results are available

//             temperature = values.temperature;
//             humidity = values.humidity;
//             pressure = values.pressure;

//             if (bme680_get_results_float(&sensor, &values) == ESP_OK)
//             {
//                 // apply a reasonability check against the readings
//                 BME680_readings_are_reasonable = ((humidity <= 100.0f) && (temperature >= -60.0f) && (temperature <= 140.0f) && (pressure >= 870.0f) && (pressure <= 1090.0f));

//                 if (!BME680_readings_are_reasonable)
//                 {
//                     ESP_LOGE("BME680", "readings: Temperature: %.2f °C   Humidity: %.2f %%   Pressure: %.2f hPa", temperature, humidity, pressure);
//                     ESP_LOGE("BME680", "the above readings are unreasonable; will try again ( %d of %d )", attempts, max_attempts);
//                 }
//             }
//             else
//             {
//                 ESP_LOGE("BME680", "could not get BME680 readings; will try again ( %d of %d )", attempts, max_attempts);
//             }
//         }
//     };

//     // power down the BME680 sensor
//     gpio_set_level(GENERAL_USER_SETTINGS_POWER_SENSOR_CONTROLLER_PIN, POWER_OFF);
//     ESP_LOGI("BME680", "BME680 powered off");

//     // release the I2C bus
//     ESP_ERROR_CHECK(i2cdev_done());

//     // The readings will be displayed later when published via MQTT, so unless you want to really want to see them here the following line can be commented out:
//     // ESP_LOGI(TAG, "readings: Temperature: %.2f °C   Humidity: %.2f %%   Pressure: %.2f hPa", temperature, humidity, pressure);
// }



void initalize_non_volatile_storage()
{

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

// void initialize_the_external_switch()
// {
//     // Initialize the external switch used to determine if the readings should be reported to PWSWeather.com

//     gpio_config_t io_conf;
//     io_conf.intr_type = GPIO_INTR_DISABLE;
//     io_conf.mode = GPIO_MODE_INPUT;
//     io_conf.pin_bit_mask = 1ULL << GENERAL_USER_SETTINGS_EXTERNAL_SWITCH_GPIO_PIN;
//     // io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
//     io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
//     io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
//     gpio_config(&io_conf);
// }



void startup_validations_and_displays()
{

    ESP_LOGI("STARTUP", "\n\n\n************************\n* Remote Station *\n************************");
// TODO: go through this block by block, don't think any is needed since sleeping for 50 seconds doesn't really save power and then wifi has to reconnect
#if CONFIG_FREERTOS_USE_TICKLESS_IDLE
    bool tickless_idle_enabled = true;
#else
    bool tickless_idle_enabled = false;
#endif

    if (GENERAL_USER_SETTINGS_REPORTING_FREQUENCY_IN_MINUTES <= 0)
    {
        ESP_LOGE("INFO", "invalid reporting frequency");
        restart_after_this_many_seconds(120);
    };

    if ((GENERAL_USER_SETTINGS_USE_AUTOMATIC_SLEEP_APPROACH < 0) || (GENERAL_USER_SETTINGS_USE_AUTOMATIC_SLEEP_APPROACH > 3))
    {
        ESP_LOGE("INFO", "invalid sleep approach");
        restart_after_this_many_seconds(120);
    };

    if ((GENERAL_USER_SETTINGS_USE_AUTOMATIC_SLEEP_APPROACH == 1) && !tickless_idle_enabled)
    {
        ESP_LOGE("INFO", "automatic light sleep requires tickless idle to be enabled");
        restart_after_this_many_seconds(120);
    }

    if ((GENERAL_USER_SETTINGS_USE_AUTOMATIC_SLEEP_APPROACH == 2) && tickless_idle_enabled)
    {
        ESP_LOGE("INFO", "manual light sleep requires tickless idle to be disabled");
        restart_after_this_many_seconds(120);
    };

    if ((GENERAL_USER_SETTINGS_USE_AUTOMATIC_SLEEP_APPROACH == 3) && tickless_idle_enabled)
    {
        ESP_LOGE("INFO", "TPL5100 sleep requires tickless idle to be disabled");
        restart_after_this_many_seconds(120);
    };

    if (GENERAL_USER_SETTINGS_USE_AUTOMATIC_SLEEP_APPROACH == 0)
        ESP_LOGI("INFO", "sleep approach: deep sleep");
    else if (GENERAL_USER_SETTINGS_USE_AUTOMATIC_SLEEP_APPROACH == 1)
        ESP_LOGI("INFO", "sleep approach: automatic light sleep");
    else if (GENERAL_USER_SETTINGS_USE_AUTOMATIC_SLEEP_APPROACH == 2)
        ESP_LOGI("INFO", "sleep approach: manual light sleep");

    ESP_LOGI("INFO", "sleep time between cycles: %d seconds", GENERAL_USER_SETTINGS_REPORTING_FREQUENCY_IN_MINUTES * 60);
}


void test()
{
   set_led(purple);
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = GENERAL_USER_SETTINGS_MQTT_BROKER_URL,
        .broker.address.port = GENERAL_USER_SETTINGS_MQTT_BROKER_PORT,
        .credentials.username = SECRET_USER_SETTINGS_MQTT_USER_ID,
        .credentials.authentication.password = SECRET_USER_SETTINGS_MQTT_USER_PASS,
        //.session.disable_keepalive = true,    // this fails on my network; it may work on yours?
        .session.keepalive = INT_MAX, // using this instead of the above
        .network.disable_auto_reconnect = true,
        .network.refresh_connection_after_ms = (GENERAL_USER_SETTINGS_REPORTING_FREQUENCY_IN_MINUTES + 1) * 60 * 1000,
    };

    MQTT_is_connected = false;
    MQTT_unknown_error = false;
    MQTT_publishing_in_progress = true;

    int64_t timeout = esp_timer_get_time() + GENERAL_USER_SETTINGS_MQTT_PUBLISHING_TIMEOUT_PERIOD * 1000000;

    // multiple attempts to connect to MQTT will be made incase the network connection has failed within the reporting period and needs to be re-established

    int attempts = 0;
    const int max_attempts = 3;

    while (!MQTT_is_connected && (attempts++ < max_attempts) && MQTT_publishing_in_progress)
    {
        if (esp_timer_get_time() < timeout)
        {
            ESP_LOGI("MQTT", "Attempting to connect to MQTT (attempt %d of %d)", attempts, max_attempts);

            MQTT_unknown_error = false;

            if (esp_timer_get_time() < timeout)
            {
                MQTT_client = esp_mqtt_client_init(&mqtt_cfg);
                esp_mqtt_client_register_event(MQTT_client, ESP_EVENT_ANY_ID, mqtt_event_handler, MQTT_client);
                ESP_LOGI("MQTT", "Starting MQTT client");
                esp_mqtt_client_start(MQTT_client);

                while ((!MQTT_is_connected) && (!MQTT_unknown_error) && (esp_timer_get_time() < timeout))
                    vTaskDelay(20 / portTICK_PERIOD_MS);
            }

            // wait for WiFi to connect (in case it has dropped out)
            while ((!WiFi_is_connected) && (esp_timer_get_time() < timeout))
                vTaskDelay(20 / portTICK_PERIOD_MS);

            if (MQTT_is_connected)
            {

                if (MQTT_publishing_in_progress)
                    send_mqtt();

                while (MQTT_publishing_in_progress && (!MQTT_unknown_error) && (esp_timer_get_time() < timeout))
                    vTaskDelay(20 / portTICK_PERIOD_MS);

              
                esp_mqtt_client_destroy(MQTT_client);
                vTaskDelay(40 / portTICK_PERIOD_MS);
            }
            else
            {
               

                esp_mqtt_client_unregister_event(MQTT_client, ESP_EVENT_ANY_ID, mqtt_event_handler);
                ESP_LOGI("MQTT", "MQTT failed to connected (attempt %d of %d)", attempts, max_attempts);

                if (attempts == max_attempts)
                    ESP_LOGE("MQTT", "Reached the MQTT connection attempt threshold");
                else
                {
                    vTaskDelay(20 / portTICK_PERIOD_MS);
                };

                MQTT_unknown_error = true;
            };

            if (esp_timer_get_time() >= timeout)
                ESP_LOGE("MQTT", "Timed out trying to connect to MQTT");
        };
    };
}

static void wait(int seconds){
   ESP_LOGI("WAIT", "Sleeping for %d seconds..", seconds);

   volatile int ms = seconds*1000;
   set_led(yellow);
   vTaskDelay(ms / portTICK_PERIOD_MS);
   ESP_LOGI("WAIT", "%s", "Wake up!");
   set_led(green);

}

void app_main(void)
{
   startup();
   //  while (1) {
      //   ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
      //   blink_led();
      //   /* Toggle the LED state */
      //   s_led_state = !s_led_state;
      //   vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
   //  }



    while (true)
    {
      test();
   //      get_bme680_readings();

   //      if (BME680_readings_are_reasonable)
   //      {
   //          publish_readings_via_MQTT();
   //      }
   //      else
   //      {
   //          ESP_LOGE("BME680", "couldn't get a valid reading from the BME680; please check the wiring;");
   //          restart_after_this_many_seconds(120);
   //      };
        wait(60);


      //   goto_sleep();
    }
}