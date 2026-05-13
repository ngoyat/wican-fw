/*
 * hw_config.h — WICAN_CUSTOM target (ESP32-S3 N16R8 + SN65HVD230)
 *
 * Rules:
 *  1. Do NOT redefine HARDWARE_VER  — already set by CMakeLists: -DHARDWARE_VER=4
 *  2. Do NOT redefine WICAN_CUSTOM  — already set by CMakeLists: -DWICAN_CUSTOM=4
 *     sleep_mode.c uses:  #if HARDWARE_VER == WICAN_CUSTOM  → resolves to 4==4 ✓
 *  3. Do NOT redefine WICAN_V210/V300/USB_V100 — commented-out in ver.h intentionally;
 *     main.c uses these as integer literals (1, 2, 3) via project_hardware_rev.
 *     They are defined here so main.c compiles without ver.h changes.
 *
 * GPIO assignments:
 *   CAN TX              → GPIO11  (TX_GPIO_NUM)
 *   CAN RX              → GPIO10  (RX_GPIO_NUM)
 *   CAN_STDBY           → GPIO9   (SN65HVD230 Rs pin; LOW=active, HIGH=standby)
 *   BLE_EN              → GPIO8   (active-LOW; pulled to GND = BLE always enabled)
 *   CONNECTED_LED       → GPIO4   (NC on custom board, safe unused output)
 *   ACTIVE_LED          → GPIO5   (NC on custom board)
 *   PWR_LED             → GPIO6   (NC on custom board)
 *   Battery ADC         → GPIO1   (ADC1_CH0; sleep_mode.c uses ADC_CHANNEL_0 directly)
 */

#pragma once
#include "driver/gpio.h"

/* ── Hardware revision integers (ver.h comments them out; define here) ───── */
/* These match the original WiCAN numbering used by main.c logic.            */
#ifndef WICAN_V210
#define WICAN_V210          1
#endif
#ifndef WICAN_V300
#define WICAN_V300          2
#endif
#ifndef WICAN_USB_V100
#define WICAN_USB_V100      3
#endif
/* WICAN_CUSTOM = 4 is already defined by CMakeLists -DWICAN_CUSTOM=4        */
/* Do NOT add #define WICAN_CUSTOM here                                       */

/* ── Human-readable hardware version string for mDNS / log ──────────────── */
#define HARDWARE_VERSION    "WiCAN-Custom-S3"

/* ── CAN bus GPIOs (used by can.c via TWAI_GENERAL_CONFIG_DEFAULT) ────────  */
#define TX_GPIO_NUM             GPIO_NUM_11
#define RX_GPIO_NUM             GPIO_NUM_10
#define CAN_STDBY_GPIO_NUM      GPIO_NUM_9

/* ── BLE enable pin (active LOW; pulled to GND → always enabled) ──────────  */
#define BLE_EN_PIN_NUM          GPIO_NUM_8

/* ── LED GPIOs (NC on custom board; assigned safe unused output pins) ──────  */
#define CONNECTED_LED_GPIO_NUM  GPIO_NUM_4
#define ACTIVE_LED_GPIO_NUM     GPIO_NUM_5
#define PWR_LED_GPIO_NUM        GPIO_NUM_6

/* ── Battery ADC GPIO (informational; sleep_mode.c uses ADC_CHANNEL_0) ────  */
/* GPIO1 = ADC1_CHANNEL_0 on ESP32-S3. Divider: 100K/10K → scale 11.0       */
#define BATT_ADC_GPIO           GPIO_NUM_1

/* ── SPIFFS mount point (used by autopid.c fopen calls) ───────────────────  */
#define FS_MOUNT_POINT          "/spiffs"
