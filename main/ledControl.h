
#pragma once
#include "driver/gpio.h"

#define LED_PIN 8

void led_init(void);
void led_on(void);
void led_off(void);
void led_blink_task(void *pvParameter);
