
idf_component_register(
    SRCS "app.c" "APP_CODE.c" "otaUpdate.c" "webServer.c" "wifiSetup.c" "find_partition.c" "write_flash.c" "ota_flag.c"

    INCLUDE_DIRS "."
    REQUIRES esp_wifi esp_http_server esp_https_ota esp_timer nvs_flash esp_system esp_driver_gpio esp_rom app_update
PRIV_REQUIRES efuse 
)

