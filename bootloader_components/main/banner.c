#include <inttypes.h>        // PRIx32 for printing
#include <stdio.h>           // ets_printf needs it
#include<string.h>
extern int ets_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

void print_secure_bootloader_message(void)

{
    ets_printf("\n");

    ets_printf("╔════════════════════════════════════════════════╗\n");
    ets_printf("║                                                ║\n");
    ets_printf("║   🔒  SECURE ESP32-C3 BOOTLOADER INITIATED  🔒 ║\n");
    ets_printf("║                                                ║\n");
    ets_printf("║   Performing Pre-Boot Integrity Checks...      ║\n");
    ets_printf("║                                                ║\n");
    ets_printf("║   [ ] Reading UDS Key from eFuse               ║\n");
    ets_printf("║   [ ] Locating Factory Application Partition   ║\n");
    ets_printf("║   [ ] Reading Application Code for Hashing     ║\n");
    ets_printf("║   [ ] Computing Digest and Validating CDI      ║\n");
    ets_printf("║                                                ║\n");
    ets_printf("╔════════════════════════════════════════════════╗\n");
    ets_printf("║   🔐 Boot security checks in progress...       ║\n");
    ets_printf("\n");
    
};
