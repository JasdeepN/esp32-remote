#include "mqtt-conn.h"

void MQTT_publish_a_reading(const char *subtopic, float value)
{

    static char topic[100];
    strcpy(topic, MQTT_MAIN_TOPIC);
    strcat(topic, "/");
    strcat(topic, subtopic);

    static char payload[13];
    sprintf(payload, "%g", value);

    ESP_LOGI("MQTT", "publish: %s %s", topic, payload);
    esp_mqtt_client_publish(MQTT_client, topic, payload, 0, MQTT_QOS, MQTT_RETAIN);
};

void MQTT_publish_all_readings()
{
    MQTT_published_messages = 0;
    MQTT_publish_a_reading("temperature", temperature);
    MQTT_publish_a_reading("humidity", humidity);
    MQTT_publish_a_reading("pressure", pressure);
};

esp_mqtt_event_handle_t event;
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{

    esp_mqtt_event_handle_t event = event_data;

    switch (event->event_id)
    {
    case MQTT_EVENT_BEFORE_CONNECT:
        ESP_LOGI("MQTT", "MQTT_EVENT_BEFORE_CONNECT");
        break;

    case MQTT_EVENT_CONNECTED:
        ESP_LOGI("MQTT", "MQTT_EVENT_CONNECTED");
        MQTT_is_connected = true;
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI("MQTT", "MQTT_EVENT_DISCONNECTED");
        MQTT_is_connected = false;
        break;

        // case MQTT_EVENT_SUBSCRIBED:
        //     ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        //     break;

        // case MQTT_EVENT_UNSUBSCRIBED:
        //     ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        //     break;

    case MQTT_EVENT_PUBLISHED:

        // ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);

        MQTT_published_messages++;
        if (MQTT_published_messages >= 1) // change if theres more then 1 msg
        {
            ESP_LOGI("MQTT", "MQTT publishing complete");
            MQTT_publishing_in_progress = false;
        };
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI("MQTT", "Confirmed %.*s received", event->topic_len, event->topic);
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE("MQTT", "MQTT_EVENT_ERROR");
        MQTT_unknown_error = true;
        break;

    default:
        ESP_LOGW("MQTT", "Other mqtt event id:%d", event->event_id);
        break;
    }
};


void publish_readings_via_MQTT()
{

    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URL,
        .broker.address.port = MQTT_BROKER_PORT,
        .credentials.username = MQTT_USER,
        .credentials.authentication.password = MQTT_PASS,
        //.session.disable_keepalive = true,    // this fails on my network; it may work on yours?
        .session.keepalive = INT_MAX, // using this instead of the above
        .network.disable_auto_reconnect = true,
        .network.refresh_connection_after_ms = (REPORTING_FREQUENCY_IN_MINUTES + 1) * 60 * 1000,
    };

    MQTT_is_connected = false;
    MQTT_unknown_error = false;
    MQTT_publishing_in_progress = true;

    int64_t timeout = esp_timer_get_time() + MQTT_PUBLISHING_TIMEOUT_PERIOD * 1000000;

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
                    MQTT_publish_all_readings();

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

void send_mqtt()
{
    static char topic[50];
    strcpy(topic, "test");
    strcat(topic, "/");
    strcat(topic, "1");

    static char payload[200]; // can shrink this 
    sprintf(payload, "%s at time %s", "hi from ESP32 C6!", get_time());

    ESP_LOGI("MQTT", "publish: %s %s", topic, payload);
    esp_mqtt_client_publish(MQTT_client, topic, payload, 0, MQTT_QOS, MQTT_RETAIN);
    
}