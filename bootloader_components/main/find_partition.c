
#include "esp_partition.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

extern int ets_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

const esp_partition_t* find_partition_by_name(const char *label)
{
    const esp_partition_t* res = NULL;

    if (strcmp(label, "cdi") == 0) {
        // CDI data partition (NVS subtype)
        res = esp_partition_find_first(
            ESP_PARTITION_TYPE_DATA,
            ESP_PARTITION_SUBTYPE_DATA_NVS,
            "cdi"
        );

    } else if (strcmp(label, "ota") == 0) {
        // OTA partition, try ota_0 first, then ota_1
        res = esp_partition_find_first(
            ESP_PARTITION_TYPE_APP,
            ESP_PARTITION_SUBTYPE_APP_OTA_0,
            "ota_0"
        );
        if (!res) {
            res = esp_partition_find_first(
                ESP_PARTITION_TYPE_APP,
                ESP_PARTITION_SUBTYPE_APP_OTA_1,
                "ota_1"
            );
        }

    } else if (strcmp(label, "factory") == 0) {
        // Factory partition
        res = esp_partition_find_first(
            ESP_PARTITION_TYPE_APP,
            ESP_PARTITION_SUBTYPE_APP_FACTORY,
            "factory"
        );

    } else {
        // Generic lookup: try app first, then data
        res = esp_partition_find_first(
            ESP_PARTITION_TYPE_APP,
            ESP_PARTITION_SUBTYPE_ANY,
            label
        );
        if (!res) {
            res = esp_partition_find_first(
                ESP_PARTITION_TYPE_DATA,
                ESP_PARTITION_SUBTYPE_ANY,
                label
            );
        }
    }

    if (!res) {
        ets_printf("[BOOT] Error: partition '%s' not found!\n", label);
        return NULL;
    }

    ets_printf("[BOOT] Partition '%s' found at 0x%08" PRIx32 ", size 0x%08" PRIx32 "\n",
               label, res->address, (uint32_t)res->size);
    return res;
}

