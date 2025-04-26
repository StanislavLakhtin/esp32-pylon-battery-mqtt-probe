/**
 * The software was developed by Stanislav Lakhtin (lakhtin.stanislav@gmail.com)
 * You can use it or parts of it freely in any non-commercial projects.
 * In case of use in commercial projects, please kindly contact the author.
 */
#ifndef PACKET_QUEUE_H
#define PACKET_QUEUE_H
#include "freertos/queue.h"
#include "sdkconfig.h"

#define PACKET_BUFFER_SIZE 256
extern QueueHandle_t packet_queue;
void init_packet_queue(void);
#endif