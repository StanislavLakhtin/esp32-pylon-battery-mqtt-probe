#pragma once
#include <stdbool.h>
#include <time.h>

#define OLED_SDA     21
#define OLED_SCL     22
#define OLED_ADDR    I2C_SSD1306_DEV_ADDR  /* 0x3C */

#define BUTTON_GPIO GPIO_NUM_19

#define COLS         16 
#define RS_ROWS      3   
#define PAGE_TITLE   0

typedef enum { SCR_WIFI, SCR_RS, SCR_MQTT, SCR_CNT } screen_t;
static bool dirty[SCR_CNT] = { true, true, true }; 
typedef enum { WIFI_DISCONNECTED, WIFI_CONNECTING, WIFI_CONNECTED } wifi_state_t;

/* Wi‑Fi */
static char wifi_ssid[COLS + 1] = "";
static wifi_state_t wifi_state = WIFI_DISCONNECTED;
static char wifi_ip[COLS + 1]   = "";

/* RS‑485 */
static char rs_log[RS_ROWS][COLS + 1];
static uint8_t rs_head = 0;

/* MQTT */
static bool    mqtt_connected = false;
static char    mqtt_ip[COLS + 1] = "";
static time_t  mqtt_last_sent = 0;

void  oled_ui_init(void);

void  oled_ui_cycle_screen(void);

void  oled_ui_update_wifi(const char *ssid, wifi_state_t state, const char *ip);

void  oled_ui_log_rs(const char *fmt, ...) __attribute__((format(printf,1,2)));

void  oled_ui_update_mqtt(bool connected, const char *broker_ip, time_t last_sent);