#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "queue.h"
QueueHandle_t packet_queue = NULL;

void init_packet_queue(void) {
    packet_queue = xQueueCreate(10, PACKET_BUFFER_SIZE);
}