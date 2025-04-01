#include "pylon_uart_handler.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "pylon_uart";

static PylonRxBuffer rx_buffers[PYLON_RX_BUFFER_COUNT];
static int active_buffer_index = 0;
static size_t active_index = 0;
static QueueHandle_t packet_queue = NULL;
static pylon_packet_callback_t user_callback = NULL;
static bool receiving = false;

static QueueHandle_t uart_event_queue = NULL;
static uart_port_t uart_used_port = UART_NUM_1;

void pylon_uart_input_byte(uint8_t byte) {
    if (!receiving) {
        if (byte == 0x7E) {
            receiving = true;
            active_index = 0;
            rx_buffers[active_buffer_index].data[active_index++] = byte;
        }
        return;
    }

    if (active_index < PYLON_MAX_PACKET_SIZE - 1) {
        rx_buffers[active_buffer_index].data[active_index++] = byte;

        if (byte == 0x0D) {
            rx_buffers[active_buffer_index].length = active_index;
            rx_buffers[active_buffer_index].ready = true;
            xQueueSend(packet_queue, &rx_buffers[active_buffer_index], 0);
            active_buffer_index = (active_buffer_index + 1) % PYLON_RX_BUFFER_COUNT;
            active_index = 0;
            receiving = false;
        }
    } else {
        receiving = false;
        active_index = 0;
    }
}

static void pylon_uart_event_task(void *param) {
    uart_event_t event;
    uint8_t byte;
    while (true) {
        if (xQueueReceive(uart_event_queue, &event, portMAX_DELAY)) {
            if (event.type == UART_DATA) {
                for (int i = 0; i < event.size; i++) {
                    if (uart_read_bytes(uart_used_port, &byte, 1, portMAX_DELAY) == 1) {
                        pylon_uart_input_byte(byte);
                    }
                }
            }
        }
    }
}

static void pylon_uart_worker_task(void *param) {
    while (true) {
        PylonRxBuffer *buf;
        if (xQueueReceive(packet_queue, &buf, portMAX_DELAY) == pdTRUE && buf->ready) {
            if (user_callback) {
                user_callback(buf->data, buf->length);
            }
            buf->ready = false;
        }
    }
}

void pylon_uart_init(uart_port_t port, pylon_packet_callback_t callback) {
    uart_used_port = port;
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    uart_param_config(port, &uart_config);
    uart_set_pin(port, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(port, 4096, 0, UART_EVENT_QUEUE_SIZE, &uart_event_queue, 0);

    packet_queue = xQueueCreate(PYLON_RX_BUFFER_COUNT, sizeof(PylonRxBuffer *));
    user_callback = callback;

    xTaskCreatePinnedToCore(pylon_uart_event_task, "pylon_uart_event_task", 4096, NULL, 10, NULL, 1);
    xTaskCreatePinnedToCore(pylon_uart_worker_task, "pylon_uart_worker", 4096, NULL, 10, NULL, 1);
}

