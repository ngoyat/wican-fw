/*
 * sleep_mode.c — Modified for WICAN_CUSTOM (ESP32-S3 N16R8)
 *
 * Changes:
 *  1. voltage_adc_ch: ADC_CHANNEL_4 -> ADC_CHANNEL_0 (GPIO1)
 *  2. read_ss_adc_voltage(): WICAN_CUSTOM voltage divider case
 *     Divider on GPIO1: R1=100K, R2=10K -> scale = 11.0
 *     *** Measure your actual resistors and update CUSTOM_VDIV_SCALE ***
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include  "freertos/queue.h"
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
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "sleep_mode.h"
#include "ble.h"
#include "esp_sleep.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "ver.h"
#include "math.h"
#include "dev_status.h"
#include "hw_config.h"

#define TAG               __func__

#define ADC_UNIT          ADC_UNIT_1
#define ADC_ATTEN         ADC_ATTEN_DB_12
#define ADC_BIT_WIDTH     SOC_ADC_DIGI_MAX_BITWIDTH

#define WAKEUP_TIME_DELAY       (200*1000)
#define MQTT_CONNECTED_BIT      BIT0
#define PUB_SUCCESS_BIT         BIT1

/*
 * GPIO1 = ADC1_CHANNEL_0 on ESP32-S3
 * All other variants keep ADC_CHANNEL_4 (GPIO5)
 */
#if HARDWARE_VER == WICAN_CUSTOM
static adc_channel_t voltage_adc_ch = ADC_CHANNEL_0;   /* GPIO1 */
#else
static adc_channel_t voltage_adc_ch = ADC_CHANNEL_4;
#endif

/*
 * Voltage divider scaling
 *   Vbat -- R1(100K) --+-- R2(10K) -- GND
 *                      +-- GPIO1 (ADC)
 *   Vbat = Vmeas_mV * (R1+R2)/R2 / 1000
 *        = Vmeas_mV * 11.0 / 1000   for 100K/10K
 *
 * *** CHANGE THIS TO MATCH YOUR ACTUAL RESISTORS ***
 * Common safe values for 12V car on 3.3V ADC:
 *   100K + 10K  -> 11.0   (max Vin=36V)  <- DEFAULT
 *    68K + 6.8K -> 11.0
 *    47K + 4.7K -> 11.0
 *   100K + 16K  -> 7.25   (WiCAN V300 original)
 */
#define CUSTOM_VDIV_SCALE   11.0f

static bool calibrated = false;
static EventGroupHandle_t s_mqtt_event_group = NULL;
static float sleep_voltage = 13.1f;
static uint8_t enable_sleep = 0;
static QueueHandle_t voltage_queue = NULL;
adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t adc1_cali_chan0_handle = NULL;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        xEventGroupSetBits(s_mqtt_event_group, MQTT_CONNECTED_BIT);
        break;
    case MQTT_EVENT_DISCONNECTED:
        xEventGroupClearBits(s_mqtt_event_group, MQTT_CONNECTED_BIT);
        break;
    case MQTT_EVENT_PUBLISHED:
        xEventGroupSetBits(s_mqtt_event_group, PUB_SUCCESS_BIT);
        break;
    default:
        break;
    }
}

static esp_mqtt_client_handle_t client = NULL;
static void mqtt_init_sleep(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = config_server_get_alert_url(),
        .broker.address.port = config_server_get_alert_port(),
        .credentials.username = config_server_get_alert_mqtt_user(),
        .credentials.authentication.password = config_server_get_alert_mqtt_pass(),
        .network.disable_auto_reconnect = true,
        .network.reconnect_timeout_ms = 4000,
    };
    if (client == NULL) {
        client = esp_mqtt_client_init(&mqtt_cfg);
        esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
        esp_mqtt_client_start(client);
    } else {
        esp_mqtt_client_reconnect(client);
    }

    EventBits_t bits = xEventGroupWaitBits(s_mqtt_event_group,
                            MQTT_CONNECTED_BIT, pdFALSE, pdFALSE,
                            pdMS_TO_TICKS(10000));
    if (bits & MQTT_CONNECTED_BIT) {
        static char pub_data[128];
        float batt_voltage = 0;
        sleep_mode_get_voltage(&batt_voltage);
        sprintf(pub_data, "{\"alert\": \"low_battery\", \"battery_voltage\": %f}", batt_voltage);
        esp_mqtt_client_publish(client, config_server_get_alert_topic(), pub_data, 0, 1, 0);
    }
}

static void calibration_init(void)
{
    esp_err_t ret = ESP_FAIL;
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id  = ADC_UNIT,
        .chan     = voltage_adc_ch,
        .atten    = ADC_ATTEN,
        .bitwidth = ADC_BIT_WIDTH,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &adc1_cali_chan0_handle);
    if (ret == ESP_OK) calibrated = true;
#endif
#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id  = ADC_UNIT,
            .atten    = ADC_ATTEN,
            .bitwidth = ADC_BIT_WIDTH,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &adc1_cali_chan0_handle);
        if (ret == ESP_OK) calibrated = true;
    }
#endif
    ESP_LOGI(TAG, "ADC calibration: %s", calibrated ? "OK" : "FAILED");
}

void oneshot_adc_init(void)
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id  = ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .atten    = ADC_ATTEN,
        .bitwidth = ADC_BIT_WIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, voltage_adc_ch, &config));
    calibration_init();
}

esp_err_t read_ss_adc_voltage(float *voltage_out)
{
    if (voltage_out == NULL) return ESP_ERR_INVALID_ARG;

    const int NUM_SAMPLES = 8;
    uint32_t sum_raw = 0, valid_samples = 0;
    uint32_t min_raw = UINT32_MAX, max_raw = 0;
    int sum_voltage = 0;

    for (int i = 0; i < NUM_SAMPLES; i++) {
        int raw_value;
        esp_err_t ret = adc_oneshot_read(adc_handle, voltage_adc_ch, &raw_value);
        if (ret == ESP_OK && raw_value < 4096) {
            int voltage = 0;
            if (calibrated)
                ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan0_handle, raw_value, &voltage));
            else
                voltage = (raw_value * 3300) / 4095;

            sum_raw += raw_value;
            sum_voltage += voltage;
            valid_samples++;
            if (raw_value < min_raw) min_raw = raw_value;
            if (raw_value > max_raw) max_raw = raw_value;
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }

    if (valid_samples > 0) {
        int avg_raw     = sum_raw     / valid_samples;
        int avg_voltage = sum_voltage / valid_samples;  /* millivolts */
        float volt_rounded = 0;

#if HARDWARE_VER == WICAN_CUSTOM
        /* GPIO1 divider: Vbat = Vmeas_mV * CUSTOM_VDIV_SCALE / 1000 */
        volt_rounded = (avg_voltage * CUSTOM_VDIV_SCALE) / 1000.0f;
#else
        if (project_hardware_rev == WICAN_V300)
            volt_rounded = (avg_voltage * 116) / (16 * 1000.0f);
        else if (project_hardware_rev == WICAN_USB_V100)
            volt_rounded = (avg_voltage * 106.49f) / (6.49f * 1000.0f);
#endif

        volt_rounded = roundf(volt_rounded * 10.0f) / 10.0f;
        *voltage_out = volt_rounded;

        ESP_LOGI(TAG, "ADC raw=%d (min=%lu max=%lu n=%lu) -> %.1f V [%s]",
                 avg_raw, min_raw, max_raw, valid_samples, *voltage_out,
                 calibrated ? "CAL" : "UNCAL");
        return ESP_OK;
    }
    return ESP_ERR_INVALID_STATE;
}

#define RUN_STATE       0
#define SLEEP_DETECTED  1
#define SLEEP_STATE     2
#define WAKEUP_STATE    3

static void adc_task(void *pvParameters)
{
    esp_err_t ret;
    static uint8_t  sleep_state = 0;
    static int64_t  sleep_detect_time = 0;
    static int64_t  wakeup_detect_time = 0;
    static int64_t  pub_time = 0;
    static float    alert_voltage = 0;
    static uint64_t alert_time;
    uint64_t        sleep_time = 0;

    alert_time = config_server_get_alert_time();
    alert_time *= 3600000000ULL;

    if (config_server_get_alert_volt(&alert_voltage) == -1)
        alert_voltage = 16.0f;

    oneshot_adc_init();

    if (config_server_get_sleep_time((uint32_t*)&sleep_time) == -1)
        sleep_time = 3;
    sleep_time *= (60 * 1000000ULL);

    while (1) {
        float battery_voltage;
        ret = read_ss_adc_voltage(&battery_voltage);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "ADC read error");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        battery_voltage += 0.2f;
        if (project_hardware_rev == WICAN_V210)
            battery_voltage = -1;

        xQueueOverwrite(voltage_queue, &battery_voltage);

        if (enable_sleep == 1) {
            switch (sleep_state) {
            case RUN_STATE:
                if (battery_voltage < sleep_voltage) {
                    sleep_detect_time = esp_timer_get_time();
                    sleep_state++;
                }
                break;

            case SLEEP_DETECTED:
                if (battery_voltage > sleep_voltage)
                    sleep_state = RUN_STATE;
                if ((esp_timer_get_time() - sleep_detect_time) > (int64_t)sleep_time)
                    sleep_state = SLEEP_STATE;
                break;

            case SLEEP_STATE:
                if (battery_voltage > sleep_voltage) {
                    wakeup_detect_time = esp_timer_get_time();
                    sleep_state = WAKEUP_STATE;
                }
                if (config_server_get_battery_alert_config()) {
                    if (battery_voltage < alert_voltage) {
                        if (((esp_timer_get_time() - pub_time) > (int64_t)alert_time) || (pub_time == 0)) {
                            pub_time = esp_timer_get_time();
                            wifi_network_init(config_server_get_alert_ssid(), config_server_get_alert_pass());
                            vTaskDelay(1000 / portTICK_PERIOD_MS);
                            uint8_t count = 0;
                            while (!wifi_network_is_connected()) {
                                vTaskDelay(1000 / portTICK_PERIOD_MS);
                                if (count++ > 10) break;
                            }
                            if (wifi_network_is_connected()) {
                                mqtt_init_sleep();
                                EventBits_t bits = xEventGroupWaitBits(s_mqtt_event_group,
                                                PUB_SUCCESS_BIT, pdFALSE, pdFALSE, pdMS_TO_TICKS(10000));
                                if (bits & PUB_SUCCESS_BIT)
                                    xEventGroupClearBits(s_mqtt_event_group, PUB_SUCCESS_BIT);
                                esp_mqtt_client_disconnect(client);
                                vTaskDelay(1000 / portTICK_PERIOD_MS);
                                wifi_network_deinit();
                            }
                        }
                    }
                }
                break;

            case WAKEUP_STATE:
                if (battery_voltage > sleep_voltage) {
                    if ((esp_timer_get_time() - wakeup_detect_time) > WAKEUP_TIME_DELAY)
                        esp_restart();
                } else {
                    dev_status_clear_bits(DEV_AWAKE_BIT);
                    dev_status_set_bits(DEV_SLEEP_BIT);
                    sleep_state = SLEEP_STATE;
                }
                break;
            }

            if (sleep_state == SLEEP_STATE) {
                can_disable();
                wifi_network_deinit();
                ble_disable();
                esp_sleep_enable_timer_wakeup(2 * 1000000);
                esp_light_sleep_start();
            } else {
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

int8_t sleep_mode_get_voltage(float *val)
{
    if (voltage_queue != NULL)
        if (xQueuePeek(voltage_queue, val, 0)) return 1;
    return -1;
}

int8_t sleep_mode_init(uint8_t enable, float sleep_volt)
{
    enable_sleep  = enable;
    sleep_voltage = sleep_volt;
    s_mqtt_event_group = xEventGroupCreate();
    voltage_queue = xQueueCreate(1, sizeof(float));
    xTaskCreate(adc_task, "adc_task", 4096, (void*)AF_INET, 5, NULL);
    return 1;
}
