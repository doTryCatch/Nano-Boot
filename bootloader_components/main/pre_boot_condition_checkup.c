//task to do 
//function to check either in update state or not 
//function to retrieve uds key
//funciton to retrieve cdi content
//funciton to retrieve applicatoin code content
//function to calculate hash of applicaiton code + uds
//function to check with retrieve cdi 

#include "pre_boot_condition_checkup.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "esp_efuse.h"
#include "mbedtls/sha256.h"
#include <string.h>
#include <stdlib.h>

static const char* TAG = "boot_check";

#define UDS_KEY_SIZE 32       // eFuse UDS key size in bytes
#define APP_READ_CHUNK 4096   // Read application in 4 KB chunks

bool perform_pre_boot_check(void)
{
    ESP_LOGI(TAG, "Pre-boot check started!");

    // ----- 1. Read UDS from eFuse block 3 -----
    uint8_t uds[UDS_KEY_SIZE] = {0};
    if (esp_efuse_read_block(ESP_EFUSE_BLK3, uds, UDS_KEY_SIZE) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read UDS from eFuse block 3");
        return false;
    }

    // ----- 2. Locate factory app partition -----
    const esp_partition_t* app_partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL
    );
    if (!app_partition) {
        ESP_LOGE(TAG, "Factory app partition not found");
        return false;
    }

    // ----- 3. Initialize SHA-256 -----
    mbedtls_sha256_context sha_ctx;
    mbedtls_sha256_init(&sha_ctx);
    mbedtls_sha256_starts_ret(&sha_ctx, 0); // 0 = SHA-256

    // ----- 4. Read and hash app binary in chunks -----
    uint8_t* buffer = malloc(APP_READ_CHUNK);
    if (!buffer) {
        ESP_LOGE(TAG, "Failed to allocate memory for app hash");
        return false;
    }

    size_t offset = 0;
    while (offset < app_partition->size) {
        size_t chunk_size = APP_READ_CHUNK;
        if (offset + chunk_size > app_partition->size) {
            chunk_size = app_partition->size - offset;
        }

        if (esp_partition_read(app_partition, offset, buffer, chunk_size) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read app partition at offset 0x%zx", offset);
            free(buffer);
            return false;
        }

        mbedtls_sha256_update_ret(&sha_ctx, buffer, chunk_size);
        offset += chunk_size;
    }
    free(buffer);

    // ----- 5. Hash UDS -----
    mbedtls_sha256_update_ret(&sha_ctx, uds, UDS_KEY_SIZE);

    // ----- 6. Finalize SHA-256 -----
    uint8_t hash[32]; // SHA-256 output
    mbedtls_sha256_finish_ret(&sha_ctx, hash);
    mbedtls_sha256_free(&sha_ctx);

    // ----- 7. Read CDI partition -----
    const esp_partition_t* cdi_partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "cdi"
    );
    if (!cdi_partition) {
        ESP_LOGE(TAG, "CDI partition not found");
        return false;
    }

    uint8_t cdi_hash[32];
    if (esp_partition_read(cdi_partition, 0, cdi_hash, sizeof(cdi_hash)) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read CDI partition");
        return false;
    }

    // ----- 8. Compare hashes -----
    if (memcmp(hash, cdi_hash, 32) != 0) {
        ESP_LOGE(TAG, "Pre-boot check failed: hash mismatch!");
        return false;
    }

    ESP_LOGI(TAG, "Pre-boot check passed: app verified.");
    return true;
}

