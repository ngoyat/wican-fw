cmake_minimum_required(VERSION 3.16)

# ── Project name & version ───────────────────────────────────────────
set(PROJECT_NAME "wican-fw")
set(PROJECT_VER  "4.00-custom")

# ── WICAN_CUSTOM hardware target ─────────────────────────────────────
# Tells hw_config.h / main.c / sleep_mode.c to use the custom GPIO map
# and SPIFFS (not FATFS), no SD, no IMU, no OBD chip.
add_compile_definitions(WICAN_CUSTOM=4 HARDWARE_VER=4)

# ── ESP-IDF project boilerplate ──────────────────────────────────────
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(${PROJECT_NAME})
