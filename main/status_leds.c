/**
 * The software was developed by Stanislav Lakhtin (lakhtin.stanislav@gmail.com)
 * You can use it or parts of it freely in any non-commercial projects.
 * In case of use in commercial projects, please kindly contact the author.
 */
#include "status_leds.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include <string.h>

#define LED_GPIO_A(i) (gpio_num_t)(25 + (i) * 2)
#define LED_GPIO_B(i) (gpio_num_t)(26 + (i) * 2)

typedef struct {
    LedState state;
    uint8_t blink_phase;
    uint32_t blink_timer_ms;
} LedControl;

static LedControl leds[LED_COUNT];

void status_leds_init(void) {
    // ⚠️ATTENTION:
    // External current limiting resistors (for example, 330 ohms)
    //  should be used for normal operation with bipolar LEDs.
    //  Internal pull-up/pull-down resistors ARE NOT SUITABLE for current control.
    for (int i = 0; i < LED_COUNT; i++) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << LED_GPIO_A(i)) | (1ULL << LED_GPIO_B(i)),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        gpio_config(&io_conf);
        gpio_set_level(LED_GPIO_A(i), 0);
        gpio_set_level(LED_GPIO_B(i), 0);
        leds[i].state = LED_OFF;
    }
}

void status_leds_set(StatusLed led, LedState state) {
    if (led >= LED_COUNT) return;
    leds[led].state = state;
    leds[led].blink_phase = 0;
    leds[led].blink_timer_ms = 0;
}

static void set_led_hw(StatusLed led, int a, int b) {
    gpio_set_level(LED_GPIO_A(led), a);
    gpio_set_level(LED_GPIO_B(led), b);
}

void status_leds_tick(void) {
    for (int i = 0; i < LED_COUNT; i++) {
        LedState s = leds[i].state;
        switch (s) {
            case LED_OFF:
                set_led_hw(i, 0, 0);
                break;
            case LED_GREEN:
                set_led_hw(i, 0, 1);
                break;
            case LED_RED:
                set_led_hw(i, 1, 0);
                break;
            case LED_BLINK_GREEN:
            case LED_BLINK_RED:
                if (leds[i].blink_timer_ms >= 500) {
                    leds[i].blink_phase ^= 1;
                    leds[i].blink_timer_ms = 0;
                } else {
                    leds[i].blink_timer_ms += 100;
                }
                if (leds[i].blink_phase) {
                    set_led_hw(i, s == LED_BLINK_GREEN ? 0 : 1,
                                  s == LED_BLINK_GREEN ? 1 : 0);
                } else {
                    set_led_hw(i, 0, 0);
                }
                break;
        }
    }
}
