
#pragma once

#include "esp_partition.h"
#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool flash_partition_write(const esp_partition_t *partition, const uint8_t *data, size_t length);

#ifdef __cplusplus
}
#endif
