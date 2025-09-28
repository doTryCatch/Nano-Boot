
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ledControl.h"
#include "wifiSetup.h"
#include "webServer.h"
#include "otaUpdate.h"

void app_main(void) {
    led_init();
    wifi_init();

    start_webserver();

    // LED blink task
    /*xTaskCreate(led_blink_task, "blink_task", 2048, NULL, 5, NULL);*/

    // Start OTA server (password-protected)
    ota_update_handler();
}

