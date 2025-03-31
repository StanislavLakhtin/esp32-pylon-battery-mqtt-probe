#ifndef PYLON_UART_HANDLER_H
#define PYLON_UART_HANDLER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "driver/uart.h"

#define PYLON_MAX_PACKET_SIZE 4608
#define PYLON_RX_BUFFER_COUNT 6
#define UART_EVENT_QUEUE_SIZE 10

typedef struct {
    char data[PYLON_MAX_PACKET_SIZE];
    size_t length;
    bool ready;
} PylonRxBuffer;

typedef void (*pylon_packet_callback_t)(const char *packet, size_t len);

void pylon_uart_init(uart_port_t port, pylon_packet_callback_t callback);
void pylon_uart_input_byte(uint8_t byte);

#endif
