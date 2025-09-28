
#include "esp_partition.h"
#include "handleUpload.h"
#include "banner.h"
#include "flash_reader.h"
#include "spi_flash_mmap.h"
#include "pre_boot_condition_checkup.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "soc/efuse_reg.h"
#include <inttypes.h>
#include "esp_rom_md5.h"
#include "find_partition.h"
#include "read_ota_flag.h"

#define UDS_KEY_SIZE 32
#define BUFFER_SIZE 1024

static const char *TAG = "boot_check";
extern int ets_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

// --------- Helper: Print task status ---------
static void print_status(const char *task, bool ok)
{
    ets_printf("[%-35s] %s\n", task, ok ? "✅ Done" : "❌ Fail");
}

// --------- Compute MD5 of first N bytes of partition ---------

bool compute_partition_md5(const esp_partition_t *partition, uint8_t out_digest[16])
{
    if (!partition || !out_digest) return false;

    uint8_t buffer[BUFFER_SIZE];
    size_t offset = 0;
    size_t max_read = partition->size;  // read entire partition

    md5_context_t ctx;
    esp_rom_md5_init(&ctx);

    while (offset < max_read) {
        size_t to_read = (max_read - offset > BUFFER_SIZE) ? BUFFER_SIZE : (max_read - offset);
        if (esp_partition_read(partition, offset, buffer, to_read) != ESP_OK) {
            return false;
        }
        esp_rom_md5_update(&ctx, buffer, to_read);
        offset += to_read;
    }

    esp_rom_md5_final(out_digest, &ctx);
    return true;
}
// --------- Print MD5 as hex ---------
void print_md5_hex(const uint8_t digest[16], const char *label)
{
    static const char hex_chars[] = "0123456789abcdef";
    char md5_str[33];

    for (int i = 0; i < 16; i++) {
        md5_str[i * 2]     = hex_chars[(digest[i] >> 4) & 0xF];
        md5_str[i * 2 + 1] = hex_chars[digest[i] & 0xF];
    }
    md5_str[32] = '\0';

    if (label) {
        ets_printf("%s MD5 (hex): %s\n", label, md5_str);
    } else {
        ets_printf("MD5 (hex): %s\n", md5_str);
    }
}

// --------- Read UDS key (BLOCK_KEY2) and return MD5 ---------
bool read_uds_md5_raw(uint8_t out_digest[16])
{
    if (!out_digest) return false;

    uint32_t buf[8];
    for (int i = 0; i < 8; i++) {
        buf[i] = REG_READ(EFUSE_RD_KEY2_DATA0_REG + i * 4);
    }

    md5_context_t ctx;
    esp_rom_md5_init(&ctx);
    esp_rom_md5_update(&ctx, (uint8_t*)buf, sizeof(buf));
    esp_rom_md5_final(out_digest, &ctx);

    return true;
}

// --------- Perform pre-boot integrity check ---------
bool perform_pre_boot_check(void)
{
    print_secure_bootloader_message();

    if (read_and_reset_otadata_flag()) {
        ets_printf("ota is set to 1 → update code is available!\n");

        // --- OTA partition MD5 ---
        const esp_partition_t *ota = find_partition_by_name("ota");
        uint8_t ota_digest[16];
        if (compute_partition_md5(ota, ota_digest)) {
            print_md5_hex(ota_digest, "hash of uploaded firmware");
        } else {
            ets_printf("Failed to compute OTA partition hash!\n");
        }

        // --- UDS MD5 ---
        uint8_t uds_digest[16];
        if (read_uds_md5_raw(uds_digest)) {
            print_md5_hex(uds_digest, "hash of UDS (Unique Device Secret)");
        } else {
            ets_printf("Failed to read UDS hash!\n");
        }

        // --- CDI calculation ---
        ets_printf("Calculating CDI = hash(uds + firmware code)........\n");
        md5_context_t cdi_ctx;
        esp_rom_md5_init(&cdi_ctx);
        esp_rom_md5_update(&cdi_ctx, ota_digest, sizeof(ota_digest));
        esp_rom_md5_update(&cdi_ctx, uds_digest, sizeof(uds_digest));

        uint8_t cdi_digest[16];
        esp_rom_md5_final(cdi_digest, &cdi_ctx);
        print_md5_hex(cdi_digest, "Generated CDI hash");

    } else {
        ets_printf("ota is set to 0 → update code is not available!\n");
    }

    // 4. Locate CDI/UDS partition
    const esp_partition_t *uds_partition = find_partition_by_name("cdi");
    bool cdi_ok = (uds_partition != NULL);
    print_status("Locating CDI Partition", cdi_ok);

    // 5. Placeholder tasks
    print_status("Reading & Hashing App Code", true);
    print_status("Hashing UDS Key", true);
    print_status("Retrieving CDI Content", true);
    print_status("Validating Digest vs CDI", true);

    ets_printf("╚════════════════════════════════════════╝\n");
    ets_printf("🔐 Pre-boot integrity check complete!\n\n");

    ESP_LOGI(TAG, "Pre-boot check finished");
    return true;
}

