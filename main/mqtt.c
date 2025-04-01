#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "queue.h"
#include "mqtt.h"
#include "esp_log.h"
#include "mqtt_client.h"

static const char *TAG = "mqtt";
static esp_mqtt_client_handle_t client = NULL;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;
    switch ((esp_mqtt_event_id_t) event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT disconnected");
            break;
        default:
            break;
    }
}

static void mqtt_publish_task(void *param) {
    uint8_t buffer[256];
    while (1) {
        if (xQueueReceive(packet_queue, buffer, portMAX_DELAY)) {
            esp_mqtt_client_publish(client, "/esp32/rs485", (char *) buffer, 0, 1, 0);
        }
    }
}

void mqtt_app_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://test.mosquitto.org",
        .session.protocol_ver = MQTT_PROTOCOL_V_3_1_1,
        .network.disable_auto_reconnect = false,
        .network.timeout_ms = 10000,
        .network.reconnect_timeout_ms = 10000,
        .task.priority = 5,
        .task.stack_size = 4096,
        .buffer.size = 1024,
        .buffer.out_size = 1024,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    xTaskCreate(mqtt_publish_task, "mqtt_pub", 4096, NULL, 10, NULL);
}
