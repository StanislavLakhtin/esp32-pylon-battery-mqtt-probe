/**
 * The software was developed by Stanislav Lakhtin (lakhtin.stanislav@gmail.com)
 * You can use it or parts of it freely in any non-commercial projects.
 * In case of use in commercial projects, please kindly contact the author.
 */
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
static pylon_packet_callback_t user_callback = NULL;
static intr_handle_t uart_intr_handle = NULL;
static QueueHandle_t pylon_rx_queue = NULL;
static uart_port_t uart_port = UART_NUM_2;

static uart_dev_t* get_uart_dev(uart_port_t port) {
    switch (port) {
        case UART_NUM_0: return &UART0;
        case UART_NUM_1: return &UART1;
        case UART_NUM_2: return &UART2;
        default: return NULL;
    }
}

static PylonRxBuffer* get_free_buffer(void) {
    for (int i = 0; i < PYLON_RX_BUFFER_COUNT; i++) {
        if (rx_buffers[i].state == PYLON_RXBUF_FREE) {
            rx_buffers[i].state = PYLON_RXBUF_RECEIVING;
            rx_buffers[i].length = 0;
            return &rx_buffers[i];
        }
    }
    return NULL;
}

static void uart_isr_handler(void *arg) {
    uart_dev_t *dev = get_uart_dev(uart_port);
    if (!dev) return;

    static PylonRxBuffer *active_buffer = NULL;

    while (dev->status.rxfifo_cnt) {
        uint8_t byte = (uint8_t)(dev->fifo.rw_byte);

        if (!active_buffer) {
            if (byte == 0x7E) { // SOI
                active_buffer = get_free_buffer();
                if (active_buffer) {
                    active_buffer->data[0] = byte;
                    active_buffer->length = 1;
                }
            }
            continue;
        }

        if (active_buffer->length < PYLON_MAX_PACKET_SIZE) {
            active_buffer->data[active_buffer->length++] = byte;

            if (byte == 0x0D) { // EOI
                active_buffer->state = PYLON_RXBUF_READY;
                PylonRxBuffer *to_send = active_buffer;
                BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                xQueueSendFromISR(pylon_rx_queue, &to_send, &xHigherPriorityTaskWoken);
                active_buffer = NULL;
                if (xHigherPriorityTaskWoken) {
                    portYIELD_FROM_ISR();
                }
            }
        } else {
            // Buffer overflow, discard
            active_buffer->state = PYLON_RXBUF_FREE;
            active_buffer = NULL;
        }
    }

    dev->int_clr.rxfifo_full = 1;
    dev->int_clr.rxfifo_tout = 1;
}

static void pylon_dispatch_task(void *param) {
    PylonRxBuffer *msg;
    while (1) {
        if (xQueueReceive(pylon_rx_queue, &msg, portMAX_DELAY)) {
            if (user_callback && msg->state == PYLON_RXBUF_READY) {
                ESP_LOGI(TAG, "Dispatching packet of length %zu", msg->length);
                user_callback(msg->data, msg->length);
                msg->state = PYLON_RXBUF_FREE;
            }
        }
        vTaskDelay(1); // минимальная уступка планировщику
    }
}

void pylon_uart_init(uart_port_t port, pylon_packet_callback_t callback) {
    uart_port = port;
    ESP_LOGI(TAG, "Low-level UART ISR handler initializing on UART%d", port);

    uart_config_t uart_config = RS485_UART_CONFIG(RS485_UART_BAUDRATE);
    ESP_ERROR_CHECK(uart_param_config(port, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(port, RS485_UART_TXD, RS485_UART_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    user_callback = callback;
    pylon_rx_queue = xQueueCreate(PYLON_RX_BUFFER_COUNT, sizeof(PylonRxBuffer *));
    xTaskCreatePinnedToCore(pylon_dispatch_task, "pylon_dispatch", 8192, NULL, 10, NULL, 1);

    uart_dev_t *dev = get_uart_dev(port);
    if (!dev) {
        ESP_LOGE(TAG, "Failed to get UART device for port %d", port);
        return;
    }

    dev->int_ena.val = 0;
    dev->int_clr.val = ~0;
    dev->int_ena.rxfifo_full = 1;
    dev->int_ena.rxfifo_tout = 1;

    ESP_ERROR_CHECK(esp_intr_alloc(
        uart_periph_signal[port].irq,
        ESP_INTR_FLAG_SHARED,
        uart_isr_handler,
        (void *)(intptr_t)port,
        &uart_intr_handle
    ));

    ESP_LOGI(TAG, "Low-level UART ISR handler initialized on UART%d", port);
}

void pylon_uart_input_byte(uint8_t byte) {
    // Заглушка для тестирования
}