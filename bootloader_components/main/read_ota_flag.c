
#include "esp_partition.h"
#include "esp_rom_spiflash.h"
#include <stdint.h>
#include <stdbool.h>

// Declare ets_printf for bootloader usage
extern int ets_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#define OTADATA_LABEL "otadata"
#define OTA_FLAG_ADDR 0       // First byte of otadata partition

bool read_and_reset_otadata_flag(void)
{
    const esp_partition_t* otadata = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA,
        ESP_PARTITION_SUBTYPE_DATA_OTA,
        OTADATA_LABEL
    );

    if (!otadata) {
        ets_printf("[BOOT] Error: otadata partition not found!\n");
        return false;
    }

    uint32_t flag = 0xFFFFFFFF;

    // Read first 4 bytes (must be uint32_t aligned)
    if (esp_rom_spiflash_read(otadata->address + OTA_FLAG_ADDR, &flag, sizeof(flag)) != ESP_OK) {
        ets_printf("[BOOT] Error: Failed to read OTA flag!\n");
        return false;
    }

    bool was_set = ((flag & 0xFF) == 1);  // Only lowest byte is flag
    ets_printf("[BOOT] OTA flag read: %u\n", (unsigned)(flag & 0xFF));

    if (was_set) {
        uint32_t reset = 0;

        // Erase first sector (4KB)
        if (esp_rom_spiflash_erase_area(otadata->address, 0x1000) != ESP_OK) {
            ets_printf("[BOOT] Error: Failed to erase otadata partition!\n");
        } else if (esp_rom_spiflash_write(otadata->address + OTA_FLAG_ADDR, &reset, sizeof(reset)) != ESP_OK) {
            ets_printf("[BOOT] Error: Failed to reset OTA flag!\n");
        } else {
            ets_printf("[BOOT] OTA flag reset to 0\n");
        }
    }

    return was_set;
}

