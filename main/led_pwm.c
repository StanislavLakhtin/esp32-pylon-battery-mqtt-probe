// led_fade_pwm.c
#include "led_pwm.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

static const char *TAG = "LED_FADE";

static uint8_t current_red = 0;
static uint8_t current_blue = 0;

static QueueHandle_t led_state_queue = NULL;

static void fade_channel(ledc_channel_t channel, uint8_t *current, uint8_t target, uint32_t duration_ms) {
    int step = (target > *current) ? 1 : -1;
    uint8_t range = (target > *current) ? target - *current : *current - target;
    if (range == 0) return;

    uint32_t delay_per_step = duration_ms / range;
    for (uint8_t i = 0; i < range; ++i) {
        *current += step;
        ledc_set_duty(LEDC_MODE, channel, *current);
        ledc_update_duty(LEDC_MODE, channel);
        vTaskDelay(pdMS_TO_TICKS(delay_per_step));
    }
}

static void led_task(void *arg) {
  led_state_t state = LED_STATE_OFF;
  bool blink_on = false;

  while (1) {
      if (xQueueReceive(led_state_queue, &state, pdMS_TO_TICKS(100))) {
          // new state received, handled in next loop
      }

      switch (state) {
          case LED_STATE_OFF:
              led_set_fade(0, 0);
              break;
          case LED_STATE_RED:
              led_set_fade(255, 0);
              break;
          case LED_STATE_BLUE:
              led_set_fade(0, 255);
              break;
          case LED_STATE_PURPLE:
              led_set_fade(255, 255);
              break;
          case LED_STATE_BLINK_RED:
              led_set_fade(blink_on ? 255 : 0, 0);
              break;
          case LED_STATE_BLINK_BLUE:
              led_set_fade(0, blink_on ? 255 : 0);
              break;
          case LED_STATE_BLINK_PURPLE:
              led_set_fade(blink_on ? 255 : 0, blink_on ? 255 : 0);
              break;
      }

      blink_on = !blink_on;
      vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void led_fade_pwm_init(void) {
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_RESOLUTION,
        .timer_num = LEDC_TIMER,
        .freq_hz = LEDC_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t red_channel = {
        .channel = LEDC_CHANNEL_RED,
        .duty = 0,
        .gpio_num = GPIO_RED,
        .speed_mode = LEDC_MODE,
        .hpoint = 0,
        .timer_sel = LEDC_TIMER
    };
    ledc_channel_config(&red_channel);

    ledc_channel_config_t blue_channel = {
        .channel = LEDC_CHANNEL_BLUE,
        .duty = 0,
        .gpio_num = GPIO_BLUE,
        .speed_mode = LEDC_MODE,
        .hpoint = 0,
        .timer_sel = LEDC_TIMER
    };
    ledc_channel_config(&blue_channel);

    current_red = 0;
    current_blue = 0;

    led_state_queue = xQueueCreate(5, sizeof(led_state_t));
    xTaskCreate(led_task, "led_task", 2048, NULL, 2, NULL);
}

void led_set_fade(uint8_t red_target, uint8_t blue_target) {
    if (red_target > current_red) {
        fade_channel(LEDC_CHANNEL_RED, &current_red, red_target, FADE_UP_TIME_MS);
    } else {
        fade_channel(LEDC_CHANNEL_RED, &current_red, red_target, FADE_DOWN_TIME_MS);
    }

    if (blue_target > current_blue) {
        fade_channel(LEDC_CHANNEL_BLUE, &current_blue, blue_target, FADE_UP_TIME_MS);
    } else {
        fade_channel(LEDC_CHANNEL_BLUE, &current_blue, blue_target, FADE_DOWN_TIME_MS);
    }
}

void led_set_state(led_state_t state) {
    if (led_state_queue != NULL) {
        xQueueSend(led_state_queue, &state, portMAX_DELAY);
    }
}