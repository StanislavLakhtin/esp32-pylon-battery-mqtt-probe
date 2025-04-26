/**
 * The software was developed by Stanislav Lakhtin (lakhtin.stanislav@gmail.com)
 * You can use it or parts of it freely in any non-commercial projects.
 * In case of use in commercial projects, please kindly contact the author.
 */
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "queue.h"
QueueHandle_t packet_queue = NULL;

void init_packet_queue(void) {
    packet_queue = xQueueCreate(10, PACKET_BUFFER_SIZE);
}