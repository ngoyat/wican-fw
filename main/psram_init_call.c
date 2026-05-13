/*
 * psram_init_call.c — WiCAN Pro (ESP32-S3 N16R8)
 * Calls wican_psram_check() early in startup via GCC constructor.
 * The __attribute__((constructor)) ensures this runs after hardware
 * init but before app_main, giving an early boot log of PSRAM status.
 */
#include "esp_log.h"

extern void wican_psram_check(void);

void __attribute__((constructor)) __wican_psram_init(void)
{
    wican_psram_check();
}
