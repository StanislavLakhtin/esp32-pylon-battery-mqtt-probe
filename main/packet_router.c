/**
 * The software was developed by Stanislav Lakhtin (lakhtin.stanislav@gmail.com)
 * You can use it or parts of it freely in any non-commercial projects.
 * In case of use in commercial projects, please kindly contact the author.
 */
#include "packet_router.h"
#include "pylon_packet.h"
#include "mqtt_formatter.h"
#include "mqtt_queue.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <string.h>

static const char *TAG = "packet_router";

static bool wifi_online = false;
static QueueHandle_t retry_queue = NULL;

void packet_router_set_online(bool online) {
    wifi_online = online;
    if (online && retry_queue) {
        MQTTPayload msg;
        while (xQueueReceive(retry_queue, &msg, 0) == pdTRUE) {
            mqtt_publish_enqueue(&msg);
            ESP_LOGI(TAG, "Resent deferred packet for topic %s", msg.topic);
        }
    }
}

void packet_router_init(void) {
    retry_queue = xQueueCreate(10, sizeof(MQTTPayload));
}

bool mqtt_retry_enqueue_force(QueueHandle_t q, const MQTTPayload *msg)
{
    if (xQueueSend(q, msg, 0) == pdTRUE) {
        return true;
    } else {
        ESP_LOGI(TAG, "Stored packet for later (offline mode)");    
    }

    MQTTPayload discarded;
    if (xQueueReceive(q, &discarded, 0) == pdTRUE) {
        ESP_LOGW("MQTT", "Queue full, discarding oldest message");
        return xQueueSend(q, msg, 0) == pdTRUE;
    }

    return false;
}

void my_packet_handler(const char *ascii_packet, size_t len) {
    PylonPacketRaw raw;
    if (!pylon_decode_ascii_hex(ascii_packet, len, &raw)) {
        ESP_LOGW(TAG, "Decode failed");
        return;
    }

    if (raw.cid1 != 0x46 || raw.cid2 != 0x00) {
        ESP_LOGW(TAG, "Unknown CID: %02X%02X", raw.cid1, raw.cid2);
        return;
    }

    PylonBatteryStatus status;
    if (!pylon_parse_info_payload(raw.data, raw.data_length, &status)) {
        ESP_LOGW(TAG, "Parse INFO failed");
        return;
    }

    MQTTPayload msg;
    if (!mqtt_format_info_payload(&status, &msg)) {
        ESP_LOGW(TAG, "Format MQTT payload failed");
        return;
    }

    if (wifi_online) {
        if (!mqtt_publish_enqueue(&msg)) {
            ESP_LOGW(TAG, "Failed to enqueue MQTT message");
        }
    } else {
        if (!mqtt_retry_enqueue_force(retry_queue, &msg)) {
            ESP_LOGW(TAG, "Retry queue full, failed to force enqueue");
        }
    }
}

