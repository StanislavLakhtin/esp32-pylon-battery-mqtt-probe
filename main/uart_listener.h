/**
 * The software was developed by Stanislav Lakhtin (lakhtin.stanislav@gmail.com)
 * You can use it or parts of it freely in any non-commercial projects.
 * In case of use in commercial projects, please kindly contact the author.
 */
#ifndef PYLON_UART_HANDLER_H
#define PYLON_UART_HANDLER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "driver/uart.h"
#include "hal/uart_hal.h"
#include "hal/gpio_types.h"

#define PYLON_MAX_PACKET_SIZE 4608
#define PYLON_RX_BUFFER_COUNT 2

#define RS485_UART_NUM     UART_NUM_2
#define RS485_UART_BAUDRATE 9600
#define RS485_UART_TXD     GPIO_NUM_16
#define RS485_UART_RXD     GPIO_NUM_17

#define RS485_UART_CONFIG(baudrate)            \
    {                                          \
        .baud_rate = baudrate,                 \
        .data_bits = UART_DATA_8_BITS,         \
        .parity    = UART_PARITY_DISABLE,      \
        .stop_bits = UART_STOP_BITS_1,         \
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, \
        .source_clk = UART_SCLK_APB,           \
    }

typedef enum {
    PYLON_RXBUF_FREE,
    PYLON_RXBUF_RECEIVING,
    PYLON_RXBUF_READY
} PylonRxBufferState;

typedef struct {
    PylonRxBufferState state;
    char data[PYLON_MAX_PACKET_SIZE];
    size_t length;
} PylonRxBuffer;

typedef void (*pylon_packet_callback_t)(const char *packet, size_t len);

void pylon_uart_init(uart_port_t port, pylon_packet_callback_t callback);

void pylon_uart_input_byte(uint8_t byte);

#endif // PYLON_UART_HANDLER_H
