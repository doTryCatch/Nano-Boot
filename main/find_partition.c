
#include "esp_partition.h"
#include <inttypes.h>
#include <string.h>

extern int ets_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/**
 * Find partition by label for CDI or App Backup
 * Returns pointer to esp_partition_t if found, NULL otherwise
 */
const esp_partition_t* find_partition_special(const char *label)
{
    const esp_partition_t* res = NULL;

    if (strcmp(label, "cdi") == 0) {
        // CDI data partition
        res = esp_partition_find_first(
            ESP_PARTITION_TYPE_DATA,
            ESP_PARTITION_SUBTYPE_DATA_NVS,  // CDI is NVS type
            "cdi"
        );
    } else if (strcmp(label, "app_backup") == 0) {
        // Backup application partition
        res = esp_partition_find_first(
            ESP_PARTITION_TYPE_APP,
            ESP_PARTITION_SUBTYPE_APP_OTA_0, // first OTA partition
            "app_backup"
        );
    }else if(strcmp(label,"ota_0")==0){
        res = esp_partition_find_first(
            ESP_PARTITION_TYPE_APP,
            ESP_PARTITION_SUBTYPE_APP_OTA_0, // first OTA partition
            "ota_0"
        );

    }

    if (!res) {
        ets_printf("[BOOT] Error: partition '%s' not found!\n", label);
        return NULL;
    }

    ets_printf("[BOOT] Partition '%s' found at 0x%08" PRIx32 ", size 0x%08" PRIx32 "\n",
               label, res->address, (uint32_t)res->size);

    return res;
}


