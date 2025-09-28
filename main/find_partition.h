
#pragma once

#include "esp_partition.h"
#include <stdint.h>
#include <stdbool.h>
const esp_partition_t* find_partition_special(const char *label);
