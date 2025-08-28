
#include "esp_log.h"
#include "esp_system.h"
#include "esp_partition.h"
#include "esp_efuse.h"
#include "esp_hmac.h"
#include "mbedtls/sha256.h"
#include <string.h>

static const char *TAG = "bootloader_secure";

// Simplified CDI check function
static bool check_firmware_integrity(void) {
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (!running) {
        ESP_LOGE(TAG, "No running partition");
        return false;
    }

    // Hash firmware
    uint8_t hash[32];
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts_ret(&ctx, 0);

    spi_flash_mmap_handle_t handle;
    const void *data;
    if (spi_flash_mmap(running->address, running->size, SPI_FLASH_MMAP_DATA, &data, &handle) != ESP_OK) {
        ESP_LOGE(TAG, "Map fail");
        return false;
    }

    mbedtls_sha256_update_ret(&ctx, (const unsigned char *)data, running->size);
    mbedtls_sha256_finish_ret(&ctx, hash);
    mbedtls_sha256_free(&ctx);
    spi_flash_munmap(handle);

    // Compute HMAC with UDS key
    uint8_t hmac[32];
    esp_err_t err = esp_hmac_calculate(HMAC_KEY3, hash, sizeof(hash), hmac);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HMAC calc failed");
        return false;
    }

    // Load stored CDI from flash partition "cdi"
    const esp_partition_t *cdi_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "cdi");
    if (!cdi_part) {
        ESP_LOGE(TAG, "CDI partition not found");
        return false;
    }
    uint8_t stored_cdi[32];
    esp_partition_read(cdi_part, 0, stored_cdi, sizeof(stored_cdi));

    if (memcmp(hmac, stored_cdi, 32) == 0) {
        ESP_LOGI(TAG, "Integrity check passed");
        return true;
    } else {
        ESP_LOGE(TAG, "Integrity check failed!");
        return false;
    }
}

void app_main(void) {
    if (!check_firmware_integrity()) {
        ESP_LOGE(TAG, "Restoring golden firmware (not implemented fully)");
        // TODO: Copy golden_fw to OTA0
    }
    ESP_LOGI(TAG, "Bootloader done, jumping to app...");
}
