#include "esp_err.h"
#include "esp_partition.h"
#include "esp_rom_sys.h"  // For ets_printf
#include <inttypes.h>        // PRIx32 for printing
extern int ets_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

esp_err_t read_flash_data(const esp_partition_t *partition, void *buffer, size_t buffer_size, size_t offset) {
    if (partition == NULL || buffer == NULL) {
        ets_printf("Invalid partition or buffer\n");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = esp_partition_read(partition, offset, buffer, buffer_size);
    if (err != ESP_OK) {
        ets_printf("Failed to read partition at offset 0x%x, size 0x%x: %d\n", offset, buffer_size, err);
    } else {
        ets_printf("Successfully read %d bytes from partition at offset 0x%x\n", buffer_size, offset);
    }

    return err;
}
