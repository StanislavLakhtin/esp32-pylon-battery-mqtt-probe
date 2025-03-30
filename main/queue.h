#ifndef PACKET_QUEUE_H
#define PACKET_QUEUE_H
#include "freertos/queue.h"

#define PACKET_BUFFER_SIZE 256
extern QueueHandle_t packet_queue;
void init_packet_queue(void);
#endif