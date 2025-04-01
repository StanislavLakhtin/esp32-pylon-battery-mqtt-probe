#include "pylon_uart_handler.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "soc/uart_struct.h"
#include "soc/uart_periph.h"
#include "esp_intr_alloc.h"
#include <string.h>

static const char *TAG = "pylon_uart";

static PylonRxBuffer rx_buffers[PYLON_RX_BUFFER_COUNT];
static int active_buffer_index = 0;
static size_t active_index = 0;
static pylon_packet_callback_t user_callback = NULL;
static bool receiving = false;
static intr_handle_t uart_intr_handle = NULL;
static QueueHandle_t pylon_rx_queue = NULL;

static uart_dev_t* get_uart_dev(uart_port_t port) {
    switch (port) {
        case UART_NUM_0: return &UART0;
        case UART_NUM_1: return &UART1;
        default: return NULL;
    }
}

static void IRAM_ATTR uart_isr_handler(void *arg) {
    uart_port_t port = (uart_port_t)(intptr_t)arg;
    uart_dev_t *dev = get_uart_dev(port);
    if (!dev) return;

    while (dev->status.rxfifo_cnt) {
        uint8_t byte = (uint8_t)(dev->ahb_fifo.rw_byte);

        if (!receiving) {
            if (byte == 0x7E) {
                receiving = true;
                active_index = 0;
                rx_buffers[active_buffer_index].data[active_index++] = byte;
            }
            continue;
        }

        if (active_index < PYLON_MAX_PACKET_SIZE - 1) {
            rx_buffers[active_buffer_index].data[active_index++] = byte;

            if (byte == 0x0D) {
                PylonRxBuffer *ready = &rx_buffers[active_buffer_index];
                ready->length = active_index;
                ready->ready = true;
                xQueueSendFromISR(pylon_rx_queue, &ready, NULL);

                active_buffer_index = (active_buffer_index + 1) % PYLON_RX_BUFFER_COUNT;
                active_index = 0;
                receiving = false;
            }
        } else {
            receiving = false;
            active_index = 0;
        }
    }

    dev->int_clr.rxfifo_full = 1;
    dev->int_clr.rxfifo_tout = 1;
}

static void pylon_dispatch_task(void *param) {
    PylonRxBuffer *msg;
    while (1) {
        if (xQueueReceive(pylon_rx_queue, &msg, portMAX_DELAY)) {
            if (user_callback && msg->ready) {
                user_callback(msg->data, msg->length);
                msg->ready = false;
            }
        }
    }
}

void pylon_uart_init(uart_port_t port, pylon_packet_callback_t callback) {
    uart_config_t uart_config = RS485_UART_CONFIG(RS485_UART_BAUDRATE);
    ESP_ERROR_CHECK(uart_param_config(port, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(port, RS485_UART_TXD, RS485_UART_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(port, uart_config.baud_rate, 0, 0, NULL, 0));

    user_callback = callback;
    pylon_rx_queue = xQueueCreate(PYLON_RX_BUFFER_COUNT, sizeof(PylonRxBuffer *));
    xTaskCreatePinnedToCore(pylon_dispatch_task, "pylon_dispatch", 4096, NULL, 10, NULL, 1);

    uart_dev_t *dev = get_uart_dev(port);
    if (!dev) return;

    dev->int_ena.val = 0;
    dev->int_clr.val = ~0;
    dev->int_ena.rxfifo_full = 1;
    dev->int_ena.rxfifo_tout = 1;

    ESP_ERROR_CHECK(esp_intr_alloc(
        uart_periph_signal[port].irq,
        ESP_INTR_FLAG_IRAM,
        uart_isr_handler,
        (void *)(intptr_t)port,
        &uart_intr_handle
    ));

    ESP_LOGI(TAG, "Low-level UART ISR handler initialized on UART%d", port);
}

void pylon_uart_input_byte(uint8_t byte) {
    (void)byte;
}