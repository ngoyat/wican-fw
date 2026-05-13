/*
 * psram_check.c — WiCAN Pro (ESP32-S3 N16R8)
 * Logs PSRAM initialization status at boot.
 * Called early by __wican_psram_init constructor in psram_init_call.c.
 */
#include "esp_log.h"
#include "esp_system.h"
#include "esp_psram.h"

#define TAG "PSRAM"

void wican_psram_check(void)
{
#if CONFIG_SPIRAM
    if (esp_psram_is_initialized()) {
        ESP_LOGI(TAG, "PSRAM OK - size: %u KB, free heap: %u B",
                 (unsigned)(esp_psram_get_size() / 1024),
                 (unsigned)esp_get_free_heap_size());
    } else {
        ESP_LOGE(TAG, "PSRAM NOT initialized - check module type and sdkconfig SPIRAM settings");
    }
#else
    ESP_LOGW(TAG, "PSRAM support not compiled in (CONFIG_SPIRAM not set)");
#endif
}
