#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "queue.h"
#include "wifi.h"
#include "mqtt.h"
#include "rs485.h"

void app_main(void) {
    init_packet_queue();
    wifi_init_sta();
    mqtt_app_start();
    xTaskCreate(uart_rx_task, "uart_rx", 4096, NULL, 10, NULL);
}