/*
 * main.c — WICAN_CUSTOM build entry point
 * Adapted from meatpiHQ/wican-fw for ESP32-S3 N16R8 + SN65HVD230
 *
 * Changes vs stock:
 *  - Hardware detection block sets project_hardware_rev = WICAN_V300
 *    so all V300 code paths are used (SPIFFS, no SD, no IMU)
 *  - BLE_EN GPIO8 configured with PULLDOWN → always enabled
 *  - CAN STDBY GPIO9 = dummy NC
 *  - LED GPIOs 4/5/6 = dummy NC
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>
#include "comm_server.h"
#include "lwip/sockets.h"
#include "driver/twai.h"
#include "types.h"
#include "esp_timer.h"
#include "config_server.h"
#include "realdash.h"
#include "slcan.h"
#include "can.h"
#include "ble.h"
#include "wifi_network.h"
#include "esp_mac.h"
#include "esp_ota_ops.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "sleep_mode.h"
#include "ble.h"
#include "esp_sleep.h"
#include "ver.h"
#include "dev_status.h"
#include "hw_config.h"
#include "wc_mdns.h"

#define TAG __func__

hardware_version_t project_hardware_rev = WICAN_V300;

static void ble_en_gpio_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BLE_EN_PIN_NUM),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,  /* LOW → BLE enabled */
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
}

static void led_gpio_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_CONNECTED_GPIO) |
                        (1ULL << LED_ACTIVE_GPIO)    |
                        (1ULL << LED_PWR_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(LED_CONNECTED_GPIO, 0);
    gpio_set_level(LED_ACTIVE_GPIO,    0);
    gpio_set_level(LED_PWR_GPIO,       1);
}

static void can_stdby_gpio_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CAN_STDBY_GPIO_NUM),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(CAN_STDBY_GPIO_NUM, 1); /* transceiver standby until CAN enabled */
}

void app_main(void)
{
    ESP_LOGI(TAG, "WiCAN CUSTOM v%s — ESP32-S3 N16R8", FW_VERSION);

    /* NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* GPIO init */
    ble_en_gpio_init();
    led_gpio_init();
    can_stdby_gpio_init();

    /* Config / web server (uses SPIFFS) */
    config_server_start();

    /* CAN */
    can_init(config_server_get_can_bitrate());
    can_enable();

    /* WiFi */
    wifi_network_init(config_server_get_wifi_ssid(),
                      config_server_get_wifi_pass());

    /* mDNS */
    wc_mdns_init();

    /* Communication server (TCP/UDP SLCAN, ELM327, RealDash) */
    comm_server_init();

    /* BLE */
    if (BLE_Enabled()) {
        ble_init();
    }

    /* Battery ADC + sleep monitor */
    uint8_t  sleep_en   = config_server_get_sleep_config();
    float    sleep_volt = 0;
    if (config_server_get_sleep_voltage(&sleep_volt) == -1)
        sleep_volt = 13.1f;
    sleep_mode_init(sleep_en, sleep_volt);

    dev_status_set_bits(DEV_AWAKE_BIT);
    ESP_LOGI(TAG, "Boot complete.");
}
