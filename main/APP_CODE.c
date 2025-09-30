
#include "APP_CODE.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"   // Include GPIO driver

void led_init(void) {
    // Reset the pin to default state
    gpio_reset_pin(LED_PIN);

    // Set as output
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    // Turn LED off initially (assuming active low)
    gpio_set_level(LED_PIN, 1);
}

void led_on(void) {
    gpio_set_level(LED_PIN, 0); // ON (active low)
}

void led_off(void) {
    gpio_set_level(LED_PIN, 1); // OFF
}


