/**
 * The software was developed by Stanislav Lakhtin (lakhtin.stanislav@gmail.com)
 * You can use it or parts of it freely in any non-commercial projects.
 * In case of use in commercial projects, please kindly contact the author.
 */

#ifndef MQTT_QUEUE_H
#define MQTT_QUEUE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "mqtt_client.h"

#define MQTT_MAX_TOPIC_LEN 128
#define MQTT_MAX_PAYLOAD_LEN 1024
#define MQTT_QUEUE_SIZE 10

typedef struct {
    char topic[MQTT_MAX_TOPIC_LEN];
    char payload[MQTT_MAX_PAYLOAD_LEN];
    int qos;
    int retain;
} MQTTPayload;

void mqtt_publish_queue_init(void);

void mqtt_publish_set_client(esp_mqtt_client_handle_t client); // ← добавляем это
bool mqtt_publish_enqueue(const MQTTPayload *msg);

void mqtt_publish_task(void *param);

#endif
