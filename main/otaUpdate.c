
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include "esp_partition.h"
#include "esp_rom_md5.h"
#include "soc/efuse_reg.h"
#include <inttypes.h>
#include "ota_flag.h"
#include "find_partition.h"

extern int ets_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#define OTA_PORT 3232
#define OTA_BUFFER_SIZE 1024
#define OTA_PASSWORD "esp32pass"

/* --- Globals --- */
char g_ota_password[64] = {0};
char g_key_string[64]   = {0};

/* --- MD5 / UDS helpers --- */
static void digest_to_hex32(const uint8_t digest[16], char out_hex[33])
{
    static const char hex_chars[] = "0123456789abcdef";
    for (int i = 0; i < 16; i++) {
        out_hex[i*2]     = hex_chars[(digest[i] >> 4) & 0xF];
        out_hex[i*2 + 1] = hex_chars[digest[i] & 0xF];
    }
    out_hex[32] = '\0';
}

static void compute_uds_md5_raw(uint8_t out_digest[16])
{
    uint32_t buf[8];
    for (int i = 0; i < 8; i++) {
        buf[i] = REG_READ(EFUSE_RD_KEY2_DATA0_REG + i*4);
    }
    md5_context_t ctx;
    esp_rom_md5_init(&ctx);
    esp_rom_md5_update(&ctx, (uint8_t*)buf, 32);
    esp_rom_md5_final(out_digest, &ctx);

    char uds_hex[33];
    digest_to_hex32(out_digest, uds_hex);
    ets_printf("[OTA] UDS MD5: %s\n", uds_hex);
}

/* --- OTA password split --- */
static void split_and_assign(const char *password)
{
    char *underscore = strchr(password, '_');
    if (underscore) {
        size_t len_first = underscore - password;
        if (len_first >= sizeof(g_ota_password)) len_first = sizeof(g_ota_password)-1;
        strncpy(g_ota_password, password, len_first);
        g_ota_password[len_first] = '\0';
        strncpy(g_key_string, underscore + 1, sizeof(g_key_string)-1);
        g_key_string[sizeof(g_key_string)-1] = '\0';
    } else {
        strncpy(g_ota_password, password, sizeof(g_ota_password)-1);
        g_ota_password[sizeof(g_ota_password)-1] = '\0';
        g_key_string[0] = '\0';
    }
    ets_printf("[OTA] OTA password = %s, Key = %s\n", g_ota_password,
               g_key_string[0] ? g_key_string : "(empty)");
}

/* --- Parse OTA auth --- */
static bool parse_auth(int sock)
{
    char buf[1024] = {0};
    int total = 0, r;
    while (total < sizeof(buf)-1) {
        r = recv(sock, buf + total, 1, 0);
        if (r <= 0) break;
        total += r;
        if (buf[total-1] == '}') break;
    }
    buf[total] = '\0';

    char *start = strstr(buf, "\"auth\"");
    if (!start) return false;
    start += strlen("\"auth\"");
    while (*start == ' ' || *start == '\t') start++;
    if (*start != ':') return false;
    start++;
    while (*start == ' ' || *start == '\t' || *start == '"') start++;

    char password[128] = {0};
    int i = 0;
    while (*start != '"' && *start != '\0' && i < sizeof(password)-1) {
        password[i++] = *start++;
    }
    password[i] = '\0';

    split_and_assign(password);
    bool match = (strcmp(g_ota_password, OTA_PASSWORD) == 0);
    send(sock, match ? "OK" : "ER", 2, 0);
    return match;
}
bool compute_partition_md5(const esp_partition_t *partition, uint8_t out_digest[16])
{
    if (!partition || !out_digest) return false;

    uint8_t buffer[OTA_BUFFER_SIZE];
    size_t offset = 0;
    size_t max_read = partition->size;  // read entire partition

    md5_context_t ctx;
    esp_rom_md5_init(&ctx);

    while (offset < max_read) {
        size_t to_read = (max_read - offset > OTA_BUFFER_SIZE) ? OTA_BUFFER_SIZE : (max_read - offset);
        if (esp_partition_read(partition, offset, buffer, to_read) != ESP_OK) {
            return false;
        }
        esp_rom_md5_update(&ctx, buffer, to_read);
        offset += to_read;
    }

    esp_rom_md5_final(out_digest, &ctx);
    return true;
}
/* --- OTA firmware receiver --- */
void receive_firmware(int sock)
{
    uint8_t buf[OTA_BUFFER_SIZE];
    int r;
    size_t total = 0;

    const esp_partition_t* ota_0 = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, "ota_0");
    const esp_partition_t* cdi   = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, "cdi");

    if (!ota_0 || !cdi) {
        ets_printf("[OTA] Required partitions not found!\n");
        return;
    }

    ets_printf("[OTA] Writing firmware to partition %s @ 0x%08" PRIx32 "\n",
               ota_0->label, (uint32_t)ota_0->address);

    if (esp_partition_erase_range(ota_0, 0, ota_0->size) != ESP_OK) {
        ets_printf("[OTA] Partition erase failed!\n");
        return;
    }

    set_otadata_flag(OTA_FLAG_UPDATED);

    // --- receive firmware in chunks and write to flash ---
    while ((r = recv(sock, buf, sizeof(buf), 0)) > 0) {
        if (total + r > ota_0->size) {
            ets_printf("[OTA] Firmware size exceeds partition!\n");
            return;
        }
        if (esp_partition_write(ota_0, total, buf, r) != ESP_OK) {
            ets_printf("[OTA] Partition write failed at offset %u\n", (unsigned int)total);
            return;
        }
        total += r;
    }

    ets_printf("[OTA] Firmware received: %u bytes, now computing hash from flash...\n", (unsigned int)total);

    // --- compute firmware MD5 by reading the partition ---
    const esp_partition_t *ota=find_partition_special("ota_0");
    uint8_t fw_digest[16];
    char fw_hex[33];
    if (compute_partition_md5(ota, fw_digest)) {
       digest_to_hex32(fw_digest, fw_hex);
       ets_printf("[OTA] Firmware MD5 (from flash) = %s\n", fw_hex);


    }else{
         ets_printf("Failed to compute OTA partition hash!\n");
    }

    

        // --- compute UDS MD5 ---
    uint8_t uds_digest[16];
    compute_uds_md5_raw(uds_digest);

    char uds_hex[33];
    digest_to_hex32(uds_digest, uds_hex);
    ets_printf("[OTA] UDS MD5 = %s\n", uds_hex);

    // --- compute CDI MD5 ---
    if (strcmp(g_key_string, "hackifyoucan") == 0) {
        ets_printf("[OTA] Computing CDI...\n");

        md5_context_t cdi_ctx;
        esp_rom_md5_init(&cdi_ctx);
        esp_rom_md5_update(&cdi_ctx, fw_digest, 16);
        esp_rom_md5_update(&cdi_ctx, uds_digest, 16);

        uint8_t cdi_digest[16];
        esp_rom_md5_final(cdi_digest, &cdi_ctx);

        char cdi_hex[33];
        digest_to_hex32(cdi_digest, cdi_hex);
        ets_printf("[OTA] CDI MD5 = %s\n", cdi_hex);

        // write CDI to partition
        if (esp_partition_erase_range(cdi, 0, cdi->size) != ESP_OK ||
            esp_partition_write(cdi, 0, cdi_digest, sizeof(cdi_digest)) != ESP_OK) {
            ets_printf("[OTA] CDI write failed!\n");
        } else {
            ets_printf("[OTA] CDI written to partition %s\n", cdi->label);
        }
    }

    ets_printf("[OTA] OTA update complete, rebooting...\n");
    esp_restart();
}

/* --- OTA task --- */
static void ota_task(void *pvParam)
{
    (void)pvParam;

    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) { vTaskDelete(NULL); return; }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(OTA_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listen_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0 ||
        listen(listen_sock, 1) < 0) {
        close(listen_sock);
        vTaskDelete(NULL);
        return;
    }

    ets_printf("[OTA] Listening on port %d\n", OTA_PORT);

    while (1) {
        int sock = accept(listen_sock, NULL, NULL);
        if (sock < 0) { vTaskDelay(pdMS_TO_TICKS(500)); continue; }

        if (parse_auth(sock)) {
            ets_printf("[OTA] Auth passed, receiving firmware...\n");
            receive_firmware(sock);
        }
        close(sock);
        ets_printf("[OTA] Connection closed\n");
    }
}

/* --- OTA handler --- */
void ota_update_handler(void)
{
    xTaskCreate(ota_task, "ota_task", 8192, NULL, 5, NULL);
}

