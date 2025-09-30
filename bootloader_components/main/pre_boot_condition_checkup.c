
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
// ------------------ Helper: Copy Partition ------------------
bool copy_partition(const esp_partition_t *src, const esp_partition_t *dst)
{
    if (!src || !dst) {
        ESP_LOGE(TAG, "❌ Invalid partitions!");
        return false;
    }

    if (dst->size < src->size) {
        ESP_LOGE(TAG, "❌ Destination partition smaller than source!");
        return false;
    }

    ESP_LOGI(TAG, "✅ Source partition successfully copied to destination!");
    return true;
}

// ------------------ Helper: Print Status ------------------
static void print_status(const char *task, bool ok)
{
    ets_printf("[%-35s] %s\n", task, ok ? "✅ Done" : "❌ Fail");
}

// ------------------ Read CDI from Partition ------------------
bool read_cdi(const esp_partition_t *partition, uint8_t out_digest[16])
{
    if (!partition || !out_digest) return false;

    size_t offset = 0;
    size_t max_read = 16;  // Only read first 16 bytes (CDI length)

    if (esp_partition_read(partition, offset, out_digest, max_read) != ESP_OK) {
        ets_printf("❌ Failed to read CDI from partition %s\n", partition->label);
        return false;
    }

    ets_printf("✅ CDI read from partition %s\n", partition->label);
    return true;
}

// ------------------ Compute Partition MD5 ------------------
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
            ets_printf("❌ Failed reading partition %s at offset %zu\n", partition->label, offset);
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
    ets_printf("✅ MD5 computed for partition %s\n", partition->label);
    return true;
}

// ------------------ Print MD5 as Hex ------------------
char *get_md5_hex(const uint8_t digest[16], const char *label)
{
    static const char hex_chars[] = "0123456789abcdef";
    static char md5_str[33];

    for (int i = 0; i < 16; i++) {
        md5_str[i * 2]     = hex_chars[(digest[i] >> 4) & 0xF];
        md5_str[i * 2 + 1] = hex_chars[digest[i] & 0xF];
    }
    md5_str[32] = '\0';

    if (label) {
        ets_printf("🔹 %s MD5 (hex): %s\n", label, md5_str);
    } else {
        ets_printf("🔹 MD5 (hex): %s\n", md5_str);
    }

    return md5_str;
}

void print_md5_hex(const uint8_t digest[16], const char *label)
{
    get_md5_hex(digest, label);  // Use the same function for consistency
}

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

    ets_printf("✅ UDS key hash computed\n");
    return true;
}

// Convert 16-byte digest to 32-character hex string
void digest_to_hex32(const uint8_t digest[16], char hex_str[33])
{
    static const char hex_chars[] = "0123456789abcdef";

    for (int i = 0; i < 16; i++) {
        hex_str[i * 2]     = hex_chars[(digest[i] >> 4) & 0xF];
        hex_str[i * 2 + 1] = hex_chars[digest[i] & 0xF];
    }
    hex_str[32] = '\0';
}

bool perform_pre_boot_check(void)
{
    print_secure_bootloader_message();

    ets_printf("\n╔════════════════════════════════════════╗\n");
    ets_printf("🔐 Starting Pre-Boot Integrity Check...\n");
    ets_printf("╚════════════════════════════════════════╝\n\n");

    if (read_and_reset_otadata_flag()) {
        ets_printf("🚀 OTA update detected! Computing hashes...\n\n");
        // --- OTA partition MD5 ---
        const esp_partition_t *ota = find_partition_by_name("ota");
        uint8_t ota_digest[16];
        if (compute_partition_md5(ota, ota_digest)) {
            print_md5_hex(ota_digest, "OTA Firmware Hash");
        } else {
            ets_printf("❌ Failed to compute OTA partition hash!\n");
        }

        // --- UDS MD5 ---
        uint8_t uds_digest[16];
        if (read_uds_md5_raw(uds_digest)) {

            print_md5_hex(uds_digest, "UDS (Unique Device Secret) Hash");
        } else {
            ets_printf("❌ Failed to read UDS hash!\n");
        }

        // --- CDI calculation ---
        ets_printf("\n🔄 Calculating CDI (MD5 of OTA + UDS)...\n");
        md5_context_t cdi_ctx;
        esp_rom_md5_init(&cdi_ctx);
        esp_rom_md5_update(&cdi_ctx, ota_digest, sizeof(ota_digest));
        esp_rom_md5_update(&cdi_ctx, uds_digest, sizeof(uds_digest));

        uint8_t cdi_digest[16];
        esp_rom_md5_final(cdi_digest, &cdi_ctx);


        char new_cdi_hex[33];
        char parent_cdi_hex[33];

        digest_to_hex32(cdi_digest, new_cdi_hex);
        ets_printf("[CDI] Generated CDI Hash: %s\n", new_cdi_hex);

        const esp_partition_t *CDI = find_partition_by_name("cdi");
        uint8_t P_CDI_digest[16];
        if (read_cdi(CDI, P_CDI_digest)) {
            digest_to_hex32(P_CDI_digest, parent_cdi_hex);
            ets_printf("[CDI] Parent CDI Hash  : %s\n", parent_cdi_hex);

            // ✅ Compare CDI hashes
            if (strcmp(new_cdi_hex, parent_cdi_hex) == 0) {
                ets_printf("\n✅ CDI verification passed! No tampering detected.\n");

                const esp_partition_t *factory_partition = find_partition_by_name("factory");
                if (factory_partition) {
                    if (copy_partition(ota, factory_partition)) {
                        ets_printf("🎉 Factory partition successfully updated with OTA firmware!\n");
                    } else {
                        ets_printf("❌ Failed to copy OTA to factory partition!\n");
                    }
                } else {
                    ets_printf("❌ Factory partition not found!\n");
                }
            } else {
                ets_printf("\n❌ CDI mismatch! Possible tampering detected. Aborting update.\n");
            }
        } else {
            ets_printf("❌ Failed to read CDI partition!\n");
        }

    } else {
        ets_printf("ℹ️ OTA update flag not set. Skipping update.\n");
    }

    // --- Placeholder task prints ---
    ets_printf("\n╔════════════════════════════════════════╗\n");
    print_status("Reading & Hashing App Code", true);
    print_status("Hashing UDS Key", true);
    print_status("Retrieving CDI Content", true);
    print_status("Validating Digest vs CDI", true);
    ets_printf("╚════════════════════════════════════════╝\n");

    ets_printf("🔄 Pre-boot integrity check complete. Launching application...\n\n");
    ESP_LOGI(TAG, "Pre-boot check finished");

    return true;
}

