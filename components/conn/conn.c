#include <stdio.h>
#include "conn.h"

enum Wifi_status WiFi_current_status;

bool WiFi_is_connected = false;
bool WiFi6_TWT_setup_successfully = false;
bool MQTT_is_connected = false;
bool light_sleep_enabled = false;

int MQTT_published_messages;
bool MQTT_publishing_in_progress;
bool MQTT_unknown_error = false;

bool going_to_sleep = false;

bool ignore_disconnect_event = false;

esp_mqtt_client_handle_t MQTT_client;

int CONNECTED_BIT = BIT0;
int DISCONNECTED_BIT = BIT1;
esp_netif_t *netif_sta = NULL;
EventGroupHandle_t wifi_event_group;

int64_t cycle_start_time = 0;

esp_pm_config_t power_management_disabled;
esp_pm_config_t power_management_enabled;


static const char *itwt_probe_status_to_str(wifi_itwt_probe_status_t status)
{
    switch (status)
    {
    case ITWT_PROBE_FAIL:
        return "itwt probe fail";
    case ITWT_PROBE_SUCCESS:
        return "itwt probe success";
    case ITWT_PROBE_TIMEOUT:
        return "itwt probe timeout";
    case ITWT_PROBE_STA_DISCONNECTED:
        return "itwt probe sta disconnected";
    default:
        return "itwt probe unknown status";
    }
}

void enable_power_save_mode(bool turn_on)
{
    if (turn_on)
        ESP_ERROR_CHECK(esp_pm_configure(&power_management_enabled));
    else
        ESP_ERROR_CHECK(esp_pm_configure(&power_management_disabled));
}

// Callback function for HTTP events
esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGI("HTTP", "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI("HTTP", "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI("HTTP", "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGI("HTTP", "HTTP_EVENT_ON_HEADER");
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGI("HTTP", "HTTP_EVENT_ON_DATA");
        ESP_LOGI("HTTP", "%.*s", evt->data_len, (char *)evt->data);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI("HTTP", "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI("HTTP", "HTTP_EVENT_DISCONNECTED");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGI("HTTP", "HTTP_EVENT_ON_REDIRECT");
    }
    return ESP_OK;
}


static void WiFi_start_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ESP_LOGI("WIFI", "Wi-Fi started");
    ESP_LOGI("WIFI", "Connecting to %s", SECRET_USER_SETTINGS_SSID);
    ESP_ERROR_CHECK(esp_wifi_connect());
}

static void WiFi_connected_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    WiFi_is_connected = false;
    ESP_LOGI("WIFI", "Wi-Fi connected");
}

static void WiFi_disconnect_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    WiFi_is_connected = false;

    if (going_to_sleep)
        ESP_LOGI("WIFI", "Wi-Fi disconnected");
    else
    {
        if (ignore_disconnect_event)
            ESP_LOGI("WIFI", "Wi-Fi disconnected, but ignoring as system is going into deep sleep");
        else
        {
            ESP_LOGI("WIFI", "Wi-Fi disconnected, reconnecting");
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            ESP_ERROR_CHECK(esp_wifi_connect());
        }
    }
}

static void setup_WIFI6_targeted_wake_time()
{

    // if the Wi-Fi connection supports it, setup Wi-Fi 6 targeted wait time

    wifi_phy_mode_t WiFiMode;

    if (esp_wifi_sta_get_negotiated_phymode(&WiFiMode) == ESP_OK)
    {
        switch (WiFiMode)
        {
        case WIFI_PHY_MODE_HE20:

            // this is ideally what we want as it allows the Wi-Fi connection to be preserved into the next cycle

            ESP_LOGI("WIFI", "802.11ax HE20");

            // setup a trigger-based announce individual TWT agreement
            wifi_config_t sta_cfg = {
                0,
            };

            esp_wifi_get_config(WIFI_IF_STA, &sta_cfg);

            esp_err_t err = ESP_OK;

            bool flow_type_announced = true;
            uint16_t twt_timeout = 5000;

            wifi_twt_setup_config_t setup_config = {
                .setup_cmd = TWT_REQUEST,
                .flow_id = 0, // announced individual TWT agreement
                .twt_id = 0,
                .flow_type = flow_type_announced ? 0 : 1,
                .min_wake_dura = ITWT_MIN_WAKE_DURATION,
                .wake_invl_expn = GENERAL_USER_SETTINGS_ITWT_WAKE_INVL_EXPN,
                .wake_invl_mant = GENERAL_USER_SETTINGS_ITWT_WAKE_INVL_MANT,
                .trigger = ITWT_TRIGGER_ENABLED,
                .timeout_time_ms = twt_timeout,
            };

            err = esp_wifi_sta_itwt_setup(&setup_config);

            if (err == ESP_OK)
                ESP_LOGI("WIFI", "Wi-Fi 6 Targeted Wake Time setup succeeded!");
            else
                ESP_LOGE("WIFI", "Wi-Fi 6 Targeted Wake Time setup failed, err:0x%x", err);

            WiFi6_TWT_setup_successfully = true;

            break;

        case WIFI_PHY_MODE_11B:
            ESP_LOGI("WIFI", "802.11b");
            break;

        case WIFI_PHY_MODE_11G:
            ESP_LOGI("WIFI", "802.11g");
            break;

        case WIFI_PHY_MODE_LR:
            ESP_LOGI("WIFI", "Low rate");
            break;

        case WIFI_PHY_MODE_HT20:
            ESP_LOGI("WIFI", "HT20");
            break;

        case WIFI_PHY_MODE_HT40:
            ESP_LOGI("WIFI", "HT40");
            break;

        default:
            ESP_LOGE("WIFI", "unknown Wi-Fi mode");
            break;
        }
    }
    else
        ESP_LOGE("WIFI", "failed to get Wi-Fi mode");

    if (!WiFi6_TWT_setup_successfully)
        ESP_LOGW("WIFI", "Wi-Fi 6 targeted wake time could not be set up");

    light_sleep_enabled = WiFi6_TWT_setup_successfully;
};

static void got_ip_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{

    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI("WIFI", "Got IP address: " IPSTR, IP2STR(&event->ip_info.ip));

    xEventGroupClearBits(wifi_event_group, DISCONNECTED_BIT);
    xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);

    // don't setup Wi-Fi 6 targeted wake time if we're going to use deep sleep
    if (GENERAL_USER_SETTINGS_USE_AUTOMATIC_SLEEP_APPROACH > 0)
        setup_WIFI6_targeted_wake_time();

    WiFi_is_connected = true;
}

static void WiFi_beacon_timeout_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ESP_LOGE("WIFI", "Beacon timeout");
}

static void WiFi6_itwt_setup_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    wifi_event_sta_itwt_setup_t *setup = (wifi_event_sta_itwt_setup_t *)event_data;
    if (setup->config.setup_cmd == TWT_ACCEPT)
    {
        /* TWT Wake Interval = TWT Wake Interval Mantissa * (2 ^ TWT Wake Interval Exponent) */
        ESP_LOGI("WIFI", "<WIFI_EVENT_ITWT_SETUP>twt_id:%d, flow_id:%d, %s, %s, wake_dura:%d, wake_invl_e:%d, wake_invl_m:%d", setup->config.twt_id,
                 setup->config.flow_id, setup->config.trigger ? "trigger-enabled" : "non-trigger-enabled", setup->config.flow_type ? "unannounced" : "announced",
                 setup->config.min_wake_dura, setup->config.wake_invl_expn, setup->config.wake_invl_mant);
        ESP_LOGI("WIFI", "<WIFI_EVENT_ITWT_SETUP>wake duration:%d us, service period:%d us", setup->config.min_wake_dura << 8, setup->config.wake_invl_mant << setup->config.wake_invl_expn);
    }
    else
    {
        ESP_LOGE("WIFI", "<WIFI_EVENT_ITWT_SETUP>twt_id:%d, unexpected setup command:%d", setup->config.twt_id, setup->config.setup_cmd);
    }
}

static void WiFi6_itwt_teardown_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    wifi_event_sta_itwt_teardown_t *teardown = (wifi_event_sta_itwt_teardown_t *)event_data;
    ESP_LOGI("WIFI", "<WIFI_EVENT_ITWT_TEARDOWN>flow_id %d%s", teardown->flow_id, (teardown->flow_id == 8) ? "(all twt)" : "");
}

static void WiFi6_itwt_suspend_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    wifi_event_sta_itwt_suspend_t *suspend = (wifi_event_sta_itwt_suspend_t *)event_data;
    ESP_LOGI("WIFI", "<WIFI_EVENT_ITWT_SUSPEND>status:%d, flow_id_bitmap:0x%x, actual_suspend_time_ms:[%lu %lu %lu %lu %lu %lu %lu %lu]",
             suspend->status, suspend->flow_id_bitmap,
             suspend->actual_suspend_time_ms[0], suspend->actual_suspend_time_ms[1], suspend->actual_suspend_time_ms[2], suspend->actual_suspend_time_ms[3],
             suspend->actual_suspend_time_ms[4], suspend->actual_suspend_time_ms[5], suspend->actual_suspend_time_ms[6], suspend->actual_suspend_time_ms[7]);
}

static void WiFi6_itwt_probe_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    wifi_event_sta_itwt_probe_t *probe = (wifi_event_sta_itwt_probe_t *)event_data;
    ESP_LOGI("WIFI", "<WIFI_EVENT_ITWT_PROBE>status:%s, reason:0x%x", itwt_probe_status_to_str(probe->status), probe->reason);
}


// static void set_static_ip(esp_netif_t *netif)
// {
// #if GENERAL_USER_SETTINGS_ENABLE_STATIC_IP
//     if (esp_netif_dhcpc_stop(netif) != ESP_OK)
//     {
//         ESP_LOGE("WIFI", "Failed to stop dhcp client");
//         return;
//     }
//     esp_netif_ip_info_t ip;
//     memset(&ip, 0, sizeof(esp_netif_ip_info_t));
//     ip.ip.addr = ipaddr_addr(GENERAL_USER_SETTINGS_STATIC_IP_ADDR);
//     ip.netmask.addr = ipaddr_addr(GENERAL_USER_SETTINGS_STATIC_IP_NETMASK_ADDR);
//     ip.gw.addr = ipaddr_addr(GENERAL_USER_SETTINGS_STATIC_IP_GW_ADDR);
//     if (esp_netif_set_ip_info(netif, &ip) != ESP_OK)
//     {
//         ESP_LOGE("WIFI", "Failed to set ip info");
//         return;
//     }
//     ESP_LOGI("WIFI", "Success setting static ip: %s, netmask: %s, gw: %s",
//              GENERAL_USER_SETTINGS_STATIC_IP_ADDR, GENERAL_USER_SETTINGS_STATIC_IP_NETMASK_ADDR, GENERAL_USER_SETTINGS_STATIC_IP_GW_ADDR);
// #endif
// }

void restart_after_this_many_seconds(int seconds)
{

    ESP_LOGE("INFO", "Delaying for %d seconds and then restarting", seconds);
    vTaskDelay(20 / portTICK_PERIOD_MS);
    esp_sleep_enable_timer_wakeup(seconds * 1000000);
    esp_deep_sleep_start();
}

void connect_to_WiFi()
{
    set_led(blue);
    
    // Check if Wi-Fi is already connected

    wifi_ap_record_t ap_info;
    esp_wifi_sta_get_ap_info(&ap_info);
    if (ap_info.rssi != 0)
    {
        ESP_LOGI("WIFI", "WIFI was previously connected, reconnecting (%d) ...", (int)ap_info.rssi);
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        ESP_ERROR_CHECK(esp_wifi_connect());
    }
    else
    {
        ESP_LOGW("WIFI", "WIFI was previously not connected, connecting ...");
        turn_on_Wifi();
    };

    // wait for Wi-Fi to connect / reconnect

    int64_t timeout = esp_timer_get_time() + GENERAL_USER_SETTINGS_WIFI_CONNECT_TIMEOUT_PERIOD * 1000000;
    while ((!WiFi_is_connected) && (esp_timer_get_time() < timeout))
        vTaskDelay(20 / portTICK_PERIOD_MS);

    if (!WiFi_is_connected)
    {
        set_led(purple);
        ESP_LOGE("WIFI", "Could not connect to WIFI within the timeout period.");
        restart_after_this_many_seconds(GENERAL_USER_SETTINGS_DEEP_SLEEP_PERIOD_WHEN_WIFI_CANNOT_CONNECT);
    };
}

static void start_wifi()
{
    // This subroutine starts the Wi-Fi
    // It will make a Wi-Fi 6 connection if possible
    // However, once the Wi-Fi event handler has been connected and an IP address has been assigned
    // the program will determine if a Wi-Fi 6 connection was actually made

    ESP_ERROR_CHECK(esp_netif_init());
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    netif_sta = esp_netif_create_default_wifi_sta();
    assert(netif_sta);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // WiFi events
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_STA_START, &WiFi_start_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &WiFi_disconnect_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &WiFi_connected_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_STA_BEACON_TIMEOUT, &WiFi_beacon_timeout_handler, NULL, NULL));

    // WiFi 6 ITWT events
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_ITWT_SETUP, &WiFi6_itwt_setup_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_ITWT_TEARDOWN, &WiFi6_itwt_teardown_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_ITWT_SUSPEND, &WiFi6_itwt_suspend_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_ITWT_PROBE, &WiFi6_itwt_probe_handler, NULL, NULL));

    // IP event
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &got_ip_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = SECRET_USER_SETTINGS_SSID,
            .password = SECRET_USER_SETTINGS_PASSWORD,
            .listen_interval = WIFI_LISTEN_INTERVAL,
            .pmf_cfg = {
                .capable = true,
                .required = false},
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT20);

    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_11AX);

    esp_wifi_set_country_code(GENERAL_USER_SETTINGS_WIFI_COUNTRY_CODE, true);

    esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
    // esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    // esp_wifi_set_ps(WIFI_PS_NONE);

   //  set_static_ip(netif_sta); // helpful if publishing to mqtt only; if publishing to pwsweather then comment this line // additional code for this is commented out above

    ESP_ERROR_CHECK(esp_wifi_start());

    uint16_t probe_timeout = 65535;
    ESP_ERROR_CHECK(esp_wifi_set_inactive_time(WIFI_IF_STA, probe_timeout));

#if CONFIG_ESP_WIFI_ENABLE_WIFI_RX_STATS
#if CONFIG_ESP_WIFI_ENABLE_WIFI_RX_MU_STATS
    esp_wifi_enable_rx_statistics(true, true);
#else
    esp_wifi_enable_rx_statistics(true, false);
#endif
#endif
#if CONFIG_ESP_WIFI_ENABLE_WIFI_TX_STATS
    esp_wifi_enable_tx_statistics(ESP_WIFI_ACI_VO, true);
    esp_wifi_enable_tx_statistics(ESP_WIFI_ACI_BE, true);
#endif

    register_system();
    register_wifi_itwt();
    register_wifi_stats();
}

void turn_on_Wifi()
{

    ESP_LOGI("WIFI", "Turn on Wi-Fi");

    WiFi_is_connected = false;
    esp_wifi_stop(); // should not be needed but seems to help?
    start_wifi();
};

void goto_sleep()
{
   set_led(yellow);
    // GENERAL_USER_SETTINGS_USE_AUTOMATIC_SLEEP_APPROACH values:
    // 0 to use deep sleep
    // 1 to use automatic light sleep; also EDF-ISP: F1 -> SDK Configuration -> Component config -> FreeRTOS -> Tickless Idle (must be checked)
    // 2 to use manual light sleep; also EDF-ISP: F1 -> SDK Configuration -> Component config -> FreeRTOS -> Tickless Idle (must be unchecked)
    // 3 to use a TPL5100 board; also EDF-ISP: F1 -> SDK Configuration -> Component config -> FreeRTOS -> Tickless Idle (must be unchecked)

    static int cycle = 1;
    int64_t cycle_time;
    int64_t sleep_time;
    cycle_time = esp_timer_get_time() - cycle_start_time;

    // TPL5100 sleep approach
    if (GENERAL_USER_SETTINGS_USE_AUTOMATIC_SLEEP_APPROACH == 3)
    {
        ignore_disconnect_event = true;
        esp_wifi_stop();
        vTaskDelay(200 / portTICK_PERIOD_MS);

        // report processing time for this cycle
        cycle_time = esp_timer_get_time();
        ESP_LOGW("PERF", "processing time: %f seconds", (float)((float)cycle_time / (float)1000000));

        ESP_LOGI("PERF", "triggering TPL5100 shutdown request");

        gpio_reset_pin(GENERAL_USER_SETTINGS_TPL5100_DONE_GPIO_PIN);
        gpio_set_direction(GENERAL_USER_SETTINGS_TPL5100_DONE_GPIO_PIN, GPIO_MODE_OUTPUT);

        gpio_set_level(GENERAL_USER_SETTINGS_TPL5100_DONE_GPIO_PIN, LOW);
        gpio_set_level(GENERAL_USER_SETTINGS_TPL5100_DONE_GPIO_PIN, HIGH);

        vTaskDelay(2000 / portTICK_PERIOD_MS); // power should turn off right away

        ESP_LOGE("PERF", "You should only see this if a TPL5100 board is not connected");

        //while (true)
        //    vTaskDelay(10 / portTICK_PERIOD_MS);

        // by now the power should be off
        // if it does not go off as expected then the code below will act as a fail safe
        ESP_LOGW("PERF", "Going into deep sleep as a fail safe");
        vTaskDelay(20 / portTICK_PERIOD_MS);
        sleep_time = ((GENERAL_USER_SETTINGS_REPORTING_FREQUENCY_IN_MINUTES * 60) * 1000000) - cycle_time;
        esp_sleep_enable_timer_wakeup(sleep_time);
        esp_deep_sleep_start();
    };

    // if we have had a relatively serious problem force deep sleep rather than light sleep
    // this will effectively reset the esp32
    if (!WiFi_is_connected || !BME680_readings_are_reasonable || MQTT_unknown_error )
        light_sleep_enabled = false;

    // report processing time for this cycle (processing time excludes sleep time)

    cycle_time = esp_timer_get_time() - cycle_start_time;

    if (cycle == 1)
        ESP_LOGW("PERF", "initial startup and cycle %d processing time: %f seconds", cycle++, (float)((float)cycle_time / (float)1000000));
    else
        ESP_LOGW("PERF", "cycle %d processing time: %f seconds", cycle++, (float)((float)cycle_time / (float)1000000));

    if (light_sleep_enabled && (GENERAL_USER_SETTINGS_USE_AUTOMATIC_SLEEP_APPROACH == 1))
    // automatic light sleep approach
    {

        if (cycle_time < ((GENERAL_USER_SETTINGS_REPORTING_FREQUENCY_IN_MINUTES * 60) * 1000000))
        {

            ESP_LOGI("PERF", "begin automatic light sleep for %d seconds\n", GENERAL_USER_SETTINGS_REPORTING_FREQUENCY_IN_MINUTES * 60);

            enable_power_save_mode(true);

            // with automatic light sleep we don't actually call light sleep
            // rather we delay for the required time
            // and power management seeing the delay kicks in automatic light sleep
            const uint64_t convert_from_microseconds_to_milliseconds_by_division = 1000;
            vTaskDelay(((GENERAL_USER_SETTINGS_REPORTING_FREQUENCY_IN_MINUTES * 60 * 1000000) - cycle_time) / convert_from_microseconds_to_milliseconds_by_division / portTICK_PERIOD_MS);

            enable_power_save_mode(false);
        }
        else
            ESP_LOGI("PERF", "skipping automatic light sleep (already running late for the next cycle)");
    }

    else if (light_sleep_enabled && (GENERAL_USER_SETTINGS_USE_AUTOMATIC_SLEEP_APPROACH == 2))
    // manual light sleep approach
    {

        if (cycle_time < ((GENERAL_USER_SETTINGS_REPORTING_FREQUENCY_IN_MINUTES * 60) * 1000000))
        {
            going_to_sleep = true;

            ESP_ERROR_CHECK(esp_wifi_stop()); // turn off wifi to save power

            while (WiFi_is_connected)
                vTaskDelay(20 / portTICK_PERIOD_MS);

            ESP_LOGI("PERF", "begin manual light sleep for %d seconds\n", GENERAL_USER_SETTINGS_REPORTING_FREQUENCY_IN_MINUTES * 60);

            vTaskDelay(20 / portTICK_PERIOD_MS); // provide some time to finalize writing to the log (this is not optional if you want to see the above log entry written)

            sleep_time = ((GENERAL_USER_SETTINGS_REPORTING_FREQUENCY_IN_MINUTES * 60) * 1000000) - cycle_time;
            esp_sleep_enable_timer_wakeup(sleep_time);

            esp_light_sleep_start();

            going_to_sleep = false;

            ESP_ERROR_CHECK(esp_wifi_start()); // turn wifi back on
        }
        else
            ESP_LOGI("PERF", "skipping manual light sleep (already running late for the next cycle)");
    }

    else
    // deep sleep approach
    {
        // testing here
        ignore_disconnect_event = true;
        // esp_wifi_stop();
        // esp_wifi_deinit();

        if (cycle_time < ((GENERAL_USER_SETTINGS_REPORTING_FREQUENCY_IN_MINUTES * 60) * 1000000))
        {

            if (GENERAL_USER_SETTINGS_USE_AUTOMATIC_SLEEP_APPROACH == 0)
                ESP_LOGI("PERF", "begin deep sleep for %d seconds\n", (GENERAL_USER_SETTINGS_REPORTING_FREQUENCY_IN_MINUTES * 60)); // show as info as using deep sleep was the user's choice
            else
                ESP_LOGW("PERF", "begin deep sleep for %d seconds\n", (GENERAL_USER_SETTINGS_REPORTING_FREQUENCY_IN_MINUTES * 60)); // show as warning as using deep sleep was not the user's choice

            vTaskDelay(20 / portTICK_PERIOD_MS); // provide some time to finalize writing to the log (this is not optional if you want to see the above log entry written)

            esp_sleep_enable_timer_wakeup(((GENERAL_USER_SETTINGS_REPORTING_FREQUENCY_IN_MINUTES * 60) * 1000000) - cycle_time);
            esp_deep_sleep_start();
        }
        else
        {
            ESP_LOGI("PERF", "skipping deep sleep (already running late for the next cycle); restarting now");
            vTaskDelay(20 / portTICK_PERIOD_MS); // provide some time to finalize writing to the log (this is not optional if you want to see the above log entry written)
            esp_restart();
        }
    }

    // reset the cycle start time
    cycle_start_time = esp_timer_get_time();

    ESP_LOGI("PERF", "awake from sleep");
}
