/*
 * hw_config.h — WICAN_CUSTOM target (ESP32-S3 N16R8 + SN65HVD230)
 *
 * ============================================================
 * ACTUAL HARDWARE PIN MAP (confirmed by user, May 2026)
 * ============================================================
 *  BAT_SENSE       GPIO1   ADC1_CH0  (voltage divider 100K/10K -> scale 11.0)
 *  WAKE_BUTTON     GPIO7   active HIGH (not used by firmware yet, defined for future)
 *  CAN_TX          GPIO16  TWAI TX -> SN65HVD230 TXD
 *  CAN_RX          GPIO17  TWAI RX <- SN65HVD230 RXD
 *  CAN_STDBY       GPIO9   SN65HVD230 Rs: LOW=active, HIGH=standby
 *  BLE_EN          GPIO8   active LOW; pulled GND -> BLE always enabled
 *  CONNECTED_LED   GPIO4   NC on custom board (safe dummy output)
 *  ACTIVE_LED      GPIO5   NC on custom board
 *  PWR_LED         GPIO6   NC on custom board
 *
 * ============================================================
 * RULES — DO NOT BREAK
 * ============================================================
 *  1. Do NOT redefine HARDWARE_VER  — set by CMakeLists: -DHARDWARE_VER=4
 *  2. Do NOT redefine WICAN_CUSTOM  — set by CMakeLists: -DWICAN_CUSTOM=4
 *     sleep_mode.c:  #if HARDWARE_VER == WICAN_CUSTOM  -> 4==4  OK
 *  3. WICAN_V210/V300/USB_V100 defined with #ifndef guards so they don't
 *     clash if ver.h ever uncomments them.
 */

#pragma once
#include "driver/gpio.h"

/* ── Hardware revision integers (ver.h comments these out; needed by main.c) */
#ifndef WICAN_V210
#define WICAN_V210          1
#endif
#ifndef WICAN_V300
#define WICAN_V300          2
#endif
#ifndef WICAN_USB_V100
#define WICAN_USB_V100      3
#endif
/* WICAN_CUSTOM = 4  already defined by CMakeLists -DWICAN_CUSTOM=4          */
/* DO NOT add #define WICAN_CUSTOM here                                       */

/* ── Human-readable version string (used by main.c mDNS + log) ───────────── */
#define HARDWARE_VERSION        "WiCAN-Custom-S3"

/* ── CAN bus GPIOs ────────────────────────────────────────────────────────── */
/* Used by can.c via TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO_NUM, RX_GPIO_NUM, …) */
#define TX_GPIO_NUM             GPIO_NUM_16
#define RX_GPIO_NUM             GPIO_NUM_17
#define CAN_STDBY_GPIO_NUM      GPIO_NUM_9

/* ── BLE enable (active LOW; pull to GND = always enabled) ───────────────── */
/* main.c:  BLE_Enabled() = !gpio_get_level(BLE_EN_PIN_NUM)                  */
#define BLE_EN_PIN_NUM          GPIO_NUM_8

/* ── Wake button (active HIGH) ───────────────────────────────────────────── */
/* Not used by current firmware; defined here for future use / logging        */
#define WAKE_BTN_GPIO_NUM       GPIO_NUM_7

/* ── LED GPIOs (NC on custom board — assigned safe unused output pins) ─────  */
#define CONNECTED_LED_GPIO_NUM  GPIO_NUM_4
#define ACTIVE_LED_GPIO_NUM     GPIO_NUM_5
#define PWR_LED_GPIO_NUM        GPIO_NUM_6

/* ── Battery ADC GPIO ────────────────────────────────────────────────────── */
/* GPIO1 = ADC1_CHANNEL_0 on ESP32-S3                                         */
/* sleep_mode.c uses ADC_CHANNEL_0 directly via #if HARDWARE_VER==WICAN_CUSTOM */
/* Voltage divider: Vbat──100K──+──10K──GND; scale = 11.0; max safe Vin=36V  */
#define BATT_ADC_GPIO           GPIO_NUM_1
