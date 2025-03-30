#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "rs485.h"
#include "queue.h"
#include "driver/uart.h"
#include "esp_log.h"

#define UART_PORT UART_NUM_1
#define BUF_SIZE 256
#define RD_BUF_SIZE BUF_SIZE

void uart_rx_task(void *arg) {
    uint8_t* data = (uint8_t*) malloc(RD_BUF_SIZE);
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_PORT, &uart_config);
    uart_driver_install(UART_PORT, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_set_pin(UART_PORT, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    while (1) {
        int len = uart_read_bytes(UART_PORT, data, RD_BUF_SIZE, 20 / portTICK_PERIOD_MS);
        if (len > 0) {
            xQueueSend(packet_queue, data, portMAX_DELAY);
        }
    }
    free(data);
}