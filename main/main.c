/**
 * The software was developed by Stanislav Lakhtin (lakhtin.stanislav@gmail.com)
 * You can use it or parts of it freely in any non-commercial projects.
 * In case of use in commercial projects, please kindly contact the author.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mqtt_client.h"
#include "esp_wifi.h"
#include "esp_netif.h"

#include "led_pwm.h"
#include "wifi.h"
#include "mqtt_queue.h"
#include "time_sync.h"
#include "uart_listener.h"
#include "packet_router.h"
#include "oled_ui.h"

static const char *TAG = "main";

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t) event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected");
            packet_router_set_online(true);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT disconnected");
            packet_router_set_online(false);
            break;
        default:
            break;
    }
}

static esp_mqtt_client_handle_t mqtt_init(void) {
    ESP_LOGI("mqtt", "MQTT client initializing. Broker: %s", CONFIG_PROBE_MQTT_BROKER_URI);
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_PROBE_MQTT_BROKER_URI,
        .credentials.username = CONFIG_PROBE_MQTT_BROKER_USERNAME,
        .credentials.authentication.password = CONFIG_PROBE_MQTT_BROKER_PASSWORD,
        //.session.protocol_ver = MQTT_PROTOCOL_V_3_1_1,
        .network.disable_auto_reconnect = false,
        .network.timeout_ms = 10000,
        .network.reconnect_timeout_ms = 10000,
        .task.priority = 5,
        .task.stack_size = 4096,
        .buffer.size = 1024,
        .buffer.out_size = 1024,
    };
    esp_mqtt_client_handle_t mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    mqtt_publish_set_client(mqtt_client);
    return mqtt_client;
}

void app_main(void) {
    ESP_LOGI(TAG, "Initializing NVS");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    oled_ui_init();

    mqtt_publish_queue_init();
    packet_router_init();
    esp_mqtt_client_handle_t mqtt_client = mqtt_init();
    ESP_LOGI(TAG, "mqtt_client = %p", mqtt_client);

    ESP_LOGI(TAG, "Initializing WiFi and network stack");
    wifi_init_sta(mqtt_client);
    time_sync_init();

    pylon_uart_init(RS485_UART_NUM, my_packet_handler);

    ESP_LOGI(TAG, "System initialization complete");
}
