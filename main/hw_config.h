/*
 * hw_config.h — WICAN_CUSTOM target
 * ESP32-S3 N16R8 + SN65HVD230 CAN transceiver
 *
 * GPIO assignments (use exact names the stock main.c expects):
 *   CAN TX              → GPIO11
 *   CAN RX              → GPIO10
 *   CAN_STDBY_GPIO_NUM  → GPIO9   (NC — SN65HVD230 Rs tied to GND)
 *   BLE_EN_PIN_NUM      → GPIO8   (active LOW, pulled down → BLE always enabled)
 *   CONNECTED_LED       → GPIO4   (NC on custom board)
 *   ACTIVE_LED          → GPIO5   (NC on custom board)
 *   PWR_LED             → GPIO6   (NC on custom board)
 *   BATT ADC            → GPIO1   (ADC1_CH0, via R divider 100K/10K)
 */

#pragma once
#include "driver/gpio.h"

/* ── Hardware version constants (must match ver.h numbering) ── */
#define WICAN_V210          1
#define WICAN_V300          2
#define WICAN_USB_V100      3
#define WICAN_CUSTOM_HW     4   /* local alias — do NOT redefine HARDWARE_VER here */

/*
 * HARDWARE_VER is set to 4 by CMakeLists.txt via -DHARDWARE_VER=4
 * Do NOT redefine it here — causes -Werror=macro-redefined
 */

/* Human-readable string used by wc_mdns_init() */
#define HARDWARE_VERSION    "WiCAN-Custom-S3"

/* ── CAN bus ─────────────────────────────────────────────────── */
#define TX_GPIO_NUM             GPIO_NUM_11
#define RX_GPIO_NUM             GPIO_NUM_10
#define CAN_STDBY_GPIO_NUM      GPIO_NUM_9   /* NC — drive HIGH = standby, LOW = active */

/* ── BLE enable (active LOW, pulled down → always ON) ────────── */
#define BLE_EN_PIN_NUM          GPIO_NUM_8

/* ── LEDs — stock main.c uses these exact names ──────────────── */
/* NC on custom board; assign safe unused output GPIOs           */
#define CONNECTED_LED_GPIO_NUM  GPIO_NUM_4
#define ACTIVE_LED_GPIO_NUM     GPIO_NUM_5
#define PWR_LED_GPIO_NUM        GPIO_NUM_6

/* ── Battery ADC ─────────────────────────────────────────────── */
/* GPIO1 = ADC1_CHANNEL_0 on ESP32-S3                            */
/* Divider: Vbat──R1(100K)──GPIO1──R2(10K)──GND → scale=11.0    */
#define BATT_ADC_GPIO           GPIO_NUM_1

/* ── Filesystem mount point (used by autopid.c) ─────────────── */
#define FS_MOUNT_POINT          "/spiffs"

/* ── PSRAM (OPI 8 MB on N16R8 — set in menuconfig, not here) ── */
