
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Reads flash memory from `address` into `buffer` of `size` bytes
// Returns true on success, false on failure
esp_err_t read_flash_data(const esp_partition_t *partition, void *buffer, size_t buffer_size, size_t offset);
 
