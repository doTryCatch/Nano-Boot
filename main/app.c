
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

// On ESP32-C3 dev boards, built-in LED is usually GPIO 8
#define BLINK_GPIO 8  

void app_main(void)
{
    // Configure the GPIO pin
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    while (1) {
        // Turn LED on
        gpio_set_level(BLINK_GPIO, 1);
        printf("LED ON\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // Turn LED off
        gpio_set_level(BLINK_GPIO, 0);
        printf("LED OFF\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

