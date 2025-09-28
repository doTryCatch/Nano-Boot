
#include "write_flash.h"
#include <stdlib.h>
#include <string.h>

#include <inttypes.h>
extern int ets_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

bool flash_partition_write(const esp_partition_t *partition, const uint8_t *data, size_t length)
{
    if (!partition || !data) return false;

    if (length > partition->size) {
        ets_printf("[FLASH] Error: data size (%zu) exceeds partition size (0x%08" PRIx32 ")\n",
                   length, (uint32_t)partition->size);
        return false;
    }

    // Erase partition
    if (esp_partition_erase_range(partition, 0, partition->size) != ESP_OK) {
        ets_printf("[FLASH] Error: failed to erase partition '%s'\n", partition->label);
        return false;
    }

    // Write data
    if (esp_partition_write(partition, 0, data, length) != ESP_OK) {
        ets_printf("[FLASH] Error: failed to write partition '%s'\n", partition->label);
        return false;
    }

    ets_printf("[FLASH] Successfully wrote %zu bytes to partition '%s'\n", length, partition->label);
    return true;
}


