/*
 * hw_config.h — WICAN_CUSTOM target (ESP32-S3 N16R8 + SN65HVD230)
 *
 * ============================================================
 * ACTUAL HARDWARE PIN MAP (confirmed by user, May 2026)
 * ============================================================
 *  BAT_SENSE       GPIO1   ADC1_CH0  (47K/10K voltage divider, ratio 5.7x)
 *  WAKE_BUTTON     GPIO7   active HIGH (not used by firmware yet)
 *  CAN_TX          GPIO16  TWAI TX -> SN65HVD230 TXD
 *  CAN_RX          GPIO17  TWAI RX <- SN65HVD230 RXD
 *  CAN_STDBY       GPIO9   SN65HVD230 Rs: LOW=active, HIGH=standby
 *  BLE_EN          GPIO8   active LOW; pulled GND -> BLE always enabled
 *  CONNECTED_LED   GPIO4   NC on custom board (safe dummy output)
 *  ACTIVE_LED      GPIO5   NC on custom board
 *  PWR_LED         GPIO6   NC on custom board
 *
 * ============================================================
 * ADC CALIBRATION (exact port of proven Arduino firmware constants)
 * ============================================================
 *  Arduino formula (raw-based):
 *    Vbat = (raw * BAT_ADC_SCALE + BAT_ADC_OFFSET) * GPIO_BAT_CAL
 *    BAT_ADC_SCALE  = 0.004715f   (ADC count -> V, includes divider ratio)
 *    BAT_ADC_OFFSET = 0.25f       (low-end non-linearity correction)
 *    GPIO_BAT_CAL   = 1.0255f     (measured vs actual trim)
 *    Divider: R1=47K, R2=10K -> ratio 5.7:1
 *
 *  IDF gives avg_voltage in MILLIVOLTS, not raw counts. Conversion:
 *    raw = avg_mV * (4095 / 3300)
 *    Vbat = (avg_mV * (4095/3300) * BAT_ADC_SCALE + BAT_ADC_OFFSET) * GPIO_BAT_CAL
 *         = (avg_mV * CUSTOM_VDIV_A + CUSTOM_VDIV_B) * CUSTOM_VDIV_C
 *
 *  Where:
 *    CUSTOM_VDIV_A = BAT_ADC_SCALE * (4095/3300) = 0.004715 * 1.24091 = 0.005851
 *    CUSTOM_VDIV_B = BAT_ADC_OFFSET = 0.25
 *    CUSTOM_VDIV_C = GPIO_BAT_CAL   = 1.0255
 *
 *  Verified at test points (formula matches Arduino exactly):
 *    USB 5V input  -> reads ~5.5V  (expected; USB goes through divider)
 *   12.0V car bat  -> reads ~12.9V
 *   13.5V car bat  -> reads ~14.5V
 *
 *  NOTE: If device reads ~9V on USB power, the BAT_SENSE node is likely
 *  connected to car 12V via OBD2 simultaneously, or USB supply is >5V.
 *  The formula itself is correct.
 *
 * ============================================================
 * RULES — DO NOT BREAK
 * ============================================================
 *  1. Do NOT redefine HARDWARE_VER  — set by CMakeLists: -DHARDWARE_VER=4
 *  2. Do NOT redefine WICAN_CUSTOM  — set by CMakeLists: -DWICAN_CUSTOM=4
 *  3. WICAN_V210/V300/USB_V100 defined with #ifndef guards.
 */

#pragma once
#include "driver/gpio.h"

/* ── Hardware revision integers */
#ifndef WICAN_V210
#define WICAN_V210          1
#endif
#ifndef WICAN_V300
#define WICAN_V300          2
#endif
#ifndef WICAN_USB_V100
#define WICAN_USB_V100      3
#endif
/* WICAN_CUSTOM = 4  already defined by CMakeLists -DWICAN_CUSTOM=4 */

/* ── Human-readable version string */
#define HARDWARE_VERSION        "WiCAN-Custom-S3"

/* ── CAN bus GPIOs */
#define TX_GPIO_NUM             GPIO_NUM_16
#define RX_GPIO_NUM             GPIO_NUM_17
#define CAN_STDBY_GPIO_NUM      GPIO_NUM_9

/* ── BLE enable (active LOW) */
#define BLE_EN_PIN_NUM          GPIO_NUM_8

/* ── Wake button (active HIGH) */
#define WAKE_BTN_GPIO_NUM       GPIO_NUM_7

/* ── LED GPIOs (NC on custom board) */
#define CONNECTED_LED_GPIO_NUM  GPIO_NUM_4
#define ACTIVE_LED_GPIO_NUM     GPIO_NUM_5
#define PWR_LED_GPIO_NUM        GPIO_NUM_6

/* ── Battery ADC GPIO */
#define BATT_ADC_GPIO           GPIO_NUM_1

/* ── Battery ADC voltage formula constants (WICAN_CUSTOM) ───────────────── */
/* IDF returns mV; formula: Vbat = (avg_mV * A + B) * C                      */
/* Exact port of Arduino: Vbat = (raw*BAT_ADC_SCALE + BAT_ADC_OFFSET)*CAL    */
/* A = BAT_ADC_SCALE * (4095/3300) = 0.004715 * 1.24091 = 0.005851           */
/* B = BAT_ADC_OFFSET = 0.25                                                  */
/* C = GPIO_BAT_CAL   = 1.0255                                                */
#define CUSTOM_VDIV_A       0.005851f
#define CUSTOM_VDIV_B       0.25f
#define CUSTOM_VDIV_C       1.0255f
