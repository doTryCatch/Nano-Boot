
#include "esp_rom_uart.h"
#include "esp_rom_sys.h"
#include "esp_log.h"
#include "esp_rom_gpio.h"
#include <stdbool.h>

static const char *TAG = "upload_check";

// Multi-byte key (example: "ESP32")
static const uint8_t KEY[] = {'E','S','P','3','2'};
static const int KEY_LEN = sizeof(KEY);
// GPIO0 input register base (C3)
#define GPIO_IN_REG    (DR_REG_GPIO_BASE + 0x3C)  // GPIO_IN register


bool gpio0_is_low(void)
{
    uint32_t level = REG_READ(GPIO_IN_REG) & BIT(0); // read GPIO0 input
    ESP_LOGI(TAG, "GPIO0 level=%d", level ? 1 : 0);
    return (level == 0);
}
bool handleUpload(void)
{
    uint8_t buf;
    int key_index = 0;
    int timeout_us = 3 * 1000000;  // 3 seconds
    int elapsed = 0;
    const int step = 1000; // 1ms per iteration

    ESP_LOGI(TAG, "Waiting for upload key...");

    while (elapsed < timeout_us) {
        int len = esp_rom_output_rx_one_char(&buf);
        if (len > 0) {
            if (buf == KEY[key_index]) {
                key_index++;
                if (key_index == KEY_LEN) {
                    ESP_LOGI(TAG, "Upload key valid!");
                    return true;  // key fully matched
                }
            } else {
                key_index = 0; // reset if mismatch
            }
        }

        esp_rom_delay_us(step);
        elapsed += step;
    }

    ESP_LOGE(TAG, "Upload key not received!");
    return false;
}

