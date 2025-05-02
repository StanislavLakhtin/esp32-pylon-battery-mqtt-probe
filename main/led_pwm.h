#ifndef LED_PWM_H
#define LED_PWM_H

#include <stdint.h>

#define GPIO_RED     4
#define GPIO_BLUE    5

#define LEDC_FREQ_HZ       5000
#define LEDC_RESOLUTION    LEDC_TIMER_8_BIT
#define LEDC_TIMER         LEDC_TIMER_0
#define LEDC_MODE          LEDC_HIGH_SPEED_MODE

#define LEDC_CHANNEL_RED   LEDC_CHANNEL_0
#define LEDC_CHANNEL_BLUE  LEDC_CHANNEL_1

#define FADE_UP_TIME_MS     618
#define FADE_DOWN_TIME_MS   382

typedef enum {
  LED_STATE_OFF,
  LED_STATE_RED,
  LED_STATE_BLUE,
  LED_STATE_PURPLE,
  LED_STATE_BLINK_RED,
  LED_STATE_BLINK_BLUE,
  LED_STATE_BLINK_PURPLE
} led_state_t;

void led_fade_pwm_init(void);
void led_set_fade(uint8_t red_target, uint8_t blue_target);
void led_set_state(led_state_t state);

#endif