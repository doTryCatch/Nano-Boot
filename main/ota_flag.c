
#include "esp_partition.h"
#include <stdint.h>
#include "ota_flag.h"
#include <string.h>
#include <stdio.h>
#include <esp_log.h>

extern int ets_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));


#define OTA_FLAG_UPDATED 1

// Write a single byte flag to the otadata partition
 void set_otadata_flag(uint8_t flag)
{
    const esp_partition_t* otadata = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, "otadata");

    if (!otadata) {
        ets_printf("[OTA] Error: otadata partition not found!\n");
        return;
    }

    // Erase first sector (usually enough for small flags)
    if (esp_partition_erase_range(otadata, 0, 0x1000) != ESP_OK) {
        ets_printf("[OTA] Failed to erase otadata partition\n");
        return;
    }

    // Write the flag at offset 0
    if (esp_partition_write(otadata, 0, &flag, sizeof(flag)) != ESP_OK) {
        ets_printf("[OTA] Failed to write OTA flag to otadata\n");
        return;
    }

    ets_printf("[OTA] otadata flag set to %u\n", (unsigned int)flag);
}
