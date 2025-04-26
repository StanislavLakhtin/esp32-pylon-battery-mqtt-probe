/**
 * The software was developed by Stanislav Lakhtin (lakhtin.stanislav@gmail.com)
 * You can use it or parts of it freely in any non-commercial projects.
 * In case of use in commercial projects, please kindly contact the author.
 */
#ifndef STATUS_LEDS_H
#define STATUS_LEDS_H

#include <stdint.h>

typedef enum {
    LED0_INIT,
    LED1_WIFI,
    LED2_RX,
    LED3_TX,
    LED_COUNT
} StatusLed;

typedef enum {
    LED_OFF,
    LED_GREEN,
    LED_RED,
    LED_BLINK_GREEN,
    LED_BLINK_RED
} LedState;

void status_leds_init(void);
void status_leds_set(StatusLed led, LedState state);
void status_leds_tick(void); // вызывать с периодом ~50-100мс

#endif
