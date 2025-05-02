#include "oled_ui.h"

#include "driver/i2c_master.h"
#include "ssd1306.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <esp_log.h>
#include <time.h>
#include <sys/time.h>

static const char *TAG = "i2c";

static ssd1306_handle_t oled;
static i2c_master_bus_handle_t bus;
static SemaphoreHandle_t mux;
static volatile screen_t current = SCR_WIFI;

static i2c_master_bus_handle_t i2c_bus;

static void i2c_init(void)
{
  const i2c_master_bus_config_t cfg = {
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .i2c_port = I2C_NUM_0,
      .sda_io_num = OLED_SDA,
      .scl_io_num = OLED_SCL,
      .glitch_ignore_cnt = 7,
  };
  ESP_LOGI(TAG, "I2C bus %d", cfg.i2c_port);
  ESP_ERROR_CHECK(i2c_new_master_bus(&cfg, &bus));
}

static void rs_push(const char *s)
{
  strncpy(rs_log[rs_head], s, COLS);
  rs_log[rs_head][COLS] = 0;
  rs_head = (rs_head + 1) % RS_ROWS;
}

static void draw_wifi(void)
{
  ssd1306_clear_display(oled, false);

  char line[COLS + 1];
  const char *st = (wifi_state == WIFI_DISCONNECTED) ? "Looking AP" : (wifi_state == WIFI_CONNECTING) ? "Connecting"
                                                                                                      : "Connected";
  snprintf(line, sizeof line, "WiFi: %s", st);
  ssd1306_display_text(oled, 0, line, false);

  snprintf(line, sizeof line, "SSID:%-.11s", wifi_ssid);
  ssd1306_display_text(oled, 1, line, false);

  snprintf(line, sizeof line, "IP:%-.13s", wifi_ip);
  ssd1306_display_text(oled, 2, line, false);

  time_t now;
  time(&now);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  snprintf(line, sizeof(line), "%02d:%02d:%02d",
           timeinfo.tm_hour,
           timeinfo.tm_min,
           timeinfo.tm_sec);
  ssd1306_display_text(oled, 3, line, false);
}

static void draw_rs(void)
{
  ssd1306_clear_display(oled, false);
  ssd1306_display_text(oled, PAGE_TITLE, "RS485 LOG", false);

  for (int i = 0; i < RS_ROWS; ++i)
  {
    const char *s = rs_log[(rs_head + i) % RS_ROWS];
    ssd1306_display_text(oled, 1 + i, s, false);
  }
}

static void draw_mqtt(void)
{
  ssd1306_clear_display(oled, false);
  ssd1306_display_text(oled, PAGE_TITLE, "MQTT STATUS", false);

  char line[COLS + 1];
  snprintf(line, sizeof line, "ST:%s", mqtt_connected ? "UP" : "DOWN");
  ssd1306_display_text(oled, 1, line, false);

  snprintf(line, sizeof line, "IP:%-.13s", mqtt_ip);
  ssd1306_display_text(oled, 2, line, false);

  /* сколько секунд назад */
  int diff = (int)(time(NULL) - mqtt_last_sent);
  snprintf(line, sizeof line, "LAST:%2ds", diff);
  ssd1306_display_text(oled, 3, line, false);
}

static void oled_task(void *arg)
{
  i2c_init();
  ssd1306_config_t cfg = I2C_SSD1306_128x32_CONFIG_DEFAULT;
  ssd1306_init(bus, &cfg, &oled);
  ssd1306_set_contrast(oled, 0x7F);

  for (;;)
  {
    xSemaphoreTake(mux, portMAX_DELAY);
    if (dirty[current])
    {
      switch (current)
      {
      case SCR_WIFI:
        draw_wifi();
        break;
      case SCR_RS:
        draw_rs();
        break;
      case SCR_MQTT:
        draw_mqtt();
        break;
      default:
        break;
      }
      dirty[current] = false; /* экран актуален */
    }
    xSemaphoreGive(mux);
    vTaskDelay(pdMS_TO_TICKS(250));
  }
}

void oled_ui_init(void)
{
  mux = xSemaphoreCreateMutex();
  xTaskCreate(oled_task, "oled",
              3 * 1024, NULL,
              tskIDLE_PRIORITY + 1, NULL);
}

void oled_ui_cycle_screen(void) /* вызвать из ISR кнопки */
{
  current = (current + 1) % SCR_CNT;
}

/* Wi‑Fi */
void oled_ui_update_wifi(const char *ssid, wifi_state_t st, const char *ip)
{
  if (xSemaphoreTake(mux, 0))
  {
    bool changed = false;

    if (ssid && strcmp(wifi_ssid, ssid))
    {
      strncpy(wifi_ssid, ssid, COLS);
      changed = true;
    }
    if (st != wifi_state)
    {
      wifi_state = st;
      changed = true;
    }
    if (ip && strcmp(wifi_ip, ip))
    {
      strncpy(wifi_ip, ip, COLS);
      changed = true;
    }

    if (changed)
      dirty[SCR_WIFI] = true;

    xSemaphoreGive(mux);
  }
}

/* RS‑485 */
void oled_ui_log_rs(const char *fmt, ...)
{
  char buf[COLS + 1];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof buf, fmt, ap);
  va_end(ap);

  if (xSemaphoreTake(mux, 0))
  {
    rs_push(buf);
    xSemaphoreGive(mux);
  }
}

/* MQTT */
void oled_ui_update_mqtt(bool connected, const char *ip, time_t last)
{
  if (xSemaphoreTake(mux, 0))
  {
    mqtt_connected = connected;
    strncpy(mqtt_ip, ip, COLS);
    mqtt_last_sent = last;
    xSemaphoreGive(mux);
  }
}