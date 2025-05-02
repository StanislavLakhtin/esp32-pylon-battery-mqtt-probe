/**
 * The software was developed by Stanislav Lakhtin (lakhtin.stanislav@gmail.com)
 * You can use it or parts of it freely in any non-commercial projects.
 * In case of use in commercial projects, please kindly contact the author.
 */
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "wifi.h"
#include "led_pwm.h"

#include <lwip/ip4_addr.h>

#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "oled_ui.h"

#define WIFI_SSID CONFIG_PROBE_WIFI_SSID
#define WIFI_PASS CONFIG_PROBE_WIFI_PASS
#define MAX_RETRY CONFIG_WIFI_PROV_SCAN_MAX_ENTRIES

static EventGroupHandle_t s_wifi_event_group;
static const char *TAG = "wifi";
static int s_retry_num = 0;

static char g_wifi_ssid[17] = ""; // 16 символов + '\0'
#define MAX_RETRY 5

static void event_handler(void *arg,
                          esp_event_base_t base,
                          int32_t id,
                          void *data)
{
    if (base == WIFI_EVENT)
    {
        switch (id)
        {

        case WIFI_EVENT_STA_START:
            oled_ui_update_wifi("?", WIFI_CONNECTING, "");
            esp_wifi_connect();
            break;

        case WIFI_EVENT_STA_CONNECTED:
        {
            const wifi_event_sta_connected_t *ev = data;
            size_t n = ev->ssid_len > 16 ? 16 : ev->ssid_len;
            memcpy(g_wifi_ssid, ev->ssid, n);
            g_wifi_ssid[n] = '\0';

            oled_ui_update_wifi(g_wifi_ssid, WIFI_CONNECTING, "");
            ESP_LOGI(TAG, "Connected to AP: %s", g_wifi_ssid);
            break;
        }

        case WIFI_EVENT_STA_DISCONNECTED:
        {
            oled_ui_update_wifi(g_wifi_ssid, WIFI_DISCONNECTED, "");
            ESP_LOGW(TAG, "Disconnected from AP: %s", g_wifi_ssid);

            if (s_retry_num < MAX_RETRY)
            {
                s_retry_num++;
                esp_wifi_connect();
                ESP_LOGI(TAG, "Retry %u/%u …", s_retry_num, MAX_RETRY);
            }
            else
            {
                ESP_LOGE(TAG, "Max retries reached, rebooting");
                esp_restart();
            }
            break;
        }

        default:
            break;
        }
    }

    else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP)
    {
        const ip_event_got_ip_t *ev = data;
        char ipbuf[16];
        esp_ip4addr_ntoa(&ev->ip_info.ip, ipbuf, sizeof ipbuf);

        oled_ui_update_wifi(g_wifi_ssid, WIFI_CONNECTED, ipbuf);
        s_retry_num = 0;
        ESP_LOGI(TAG, "Got IP: %s", ipbuf);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    led_set_state(LED_STATE_OFF);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_LOGI(TAG, "Trying to connect to %s...", WIFI_SSID);
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .failure_retry_cnt = 2,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");
}
