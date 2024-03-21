
idf_component_register(SRCS "conn.c" "mqtt-conn.c"
                    INCLUDE_DIRS "include" "../iperf/include"
                    REQUIRES led-control log cmd_system esp_wifi esp_timer nvs_flash lwip mqtt esp_http_client json 
                    PRIV_REQUIRES iperf protocol_examples_common time-sync)