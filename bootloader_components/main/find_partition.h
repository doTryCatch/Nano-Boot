#ifndef PARTITION_UTILS_H
#define PARTITION_UTILS_H
#include "esp_partition.h"
#include<stddef.h>
const esp_partition_t* find_partition_by_name(const char *label);
#endif
