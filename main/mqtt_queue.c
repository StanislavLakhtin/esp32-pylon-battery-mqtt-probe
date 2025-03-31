#include "mqtt_queue.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mqtt_client.h"

static const char *TAG = "mqtt_queue";
static QueueHandle_t mqtt_queue = NULL;
static esp_mqtt_client_handle_t mqtt_client_handle = NULL;

void mqtt_publish_queue_init(void) {
    mqtt_queue = xQueueCreate(MQTT_QUEUE_SIZE, sizeof(MQTTPayload));
    xTaskCreatePinnedToCore(mqtt_publish_task, "mqtt_pub_task", 4096, NULL, 8, NULL, 1);
}

void mqtt_publish_set_client(esp_mqtt_client_handle_t client) {
    mqtt_client_handle = client;
}

bool mqtt_publish_enqueue(const MQTTPayload *msg) {
    if (!mqtt_queue) return false;
    return xQueueSend(mqtt_queue, msg, 0) == pdTRUE;
}

void mqtt_publish_task(void *param) {
    MQTTPayload msg;
    while (1) {
        if (xQueueReceive(mqtt_queue, &msg, portMAX_DELAY) == pdTRUE && mqtt_client_handle) {
            int msg_id = esp_mqtt_client_publish(
                mqtt_client_handle,
                msg.topic,
                msg.payload,
                0,
                msg.qos,
                msg.retain
            );
            ESP_LOGI(TAG, "Published to %s: msg_id=%d", msg.topic, msg_id);
        }
    }
}