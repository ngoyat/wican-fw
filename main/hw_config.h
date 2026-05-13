/*
 * hw_config.h — WICAN_CUSTOM target
 * ESP32-S3 N16R8 + SN65HVD230 CAN transceiver
 *
 * GPIO assignments:
 *   CAN TX        → GPIO11
 *   CAN RX        → GPIO10
 *   CAN STDBY     → GPIO9   (dummy NC — SN65HVD230 Rs tied to GND)
 *   BLE_EN        → GPIO8   (dummy, pulled DOWN → BLE always ON)
 *   LED CONNECTED → GPIO4   (dummy NC)
 *   LED ACTIVE    → GPIO5   (dummy NC)
 *   LED POWER     → GPIO6   (dummy NC)
 *   BATT ADC      → GPIO1   (ADC1_CH0, via R divider)
 */

#pragma once
#include "driver/gpio.h"

/* ── Hardware version tag ─────────────────────────────── */
#define WICAN_V210      0
#define WICAN_V300      1
#define WICAN_USB_V100  2
#define WICAN_PRO       3
#define WICAN_CUSTOM    4

#define HARDWARE_VER    WICAN_CUSTOM

/* ── CAN bus ──────────────────────────────────────────── */
#define TX_GPIO_NUM         GPIO_NUM_11
#define RX_GPIO_NUM         GPIO_NUM_10
#define CAN_STDBY_GPIO_NUM  GPIO_NUM_9   /* dummy — NC */

/* ── BLE enable (active LOW, pulled down → always ON) ─── */
#define BLE_EN_PIN_NUM      GPIO_NUM_8

/* ── LEDs (dummy, NC) ────────────────────────────────── */
#define LED_CONNECTED_GPIO  GPIO_NUM_4
#define LED_ACTIVE_GPIO     GPIO_NUM_5
#define LED_PWR_GPIO        GPIO_NUM_6

/* ── Battery ADC ─────────────────────────────────────── */
/* GPIO1 = ADC1_CHANNEL_0 on ESP32-S3                     */
/* Wire a resistor divider: Vbat─R1─GPIO1─R2─GND          */
/* Recommended: R1=100K, R2=10K → max Vin=36V             */
#define BATT_ADC_GPIO       GPIO_NUM_1

/* ── PSRAM (OPI 8 MB on N16R8) ───────────────────────── */
#define CONFIG_SPIRAM_MODE_OCT   1
#define CONFIG_SPIRAM_SPEED_80M  1
