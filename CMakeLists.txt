cmake_minimum_required(VERSION 3.16)

# ── Project name & version ───────────────────────────────────────────
set(PROJECT_NAME "wican-fw")
set(PROJECT_VER  "4.00-custom")

# ── WICAN_CUSTOM hardware target ─────────────────────────────────────
# Tells hw_config.h / main.c / sleep_mode.c to use the custom GPIO map.
add_compile_definitions(
    WICAN_CUSTOM=4
    HARDWARE_VER=4

    # SPIFFS mount point — used by config_server.c, autopid.c, ftp.c
    # Must be a quoted string literal so adjacent-string concat works:
    #   fopen(FS_MOUNT_POINT"/config.json", "r")  ->  fopen("/spiffs/config.json", "r")
    FS_MOUNT_POINT="/spiffs"

    # Git SHA placeholder — used by config_server.c check_status_handler()
    # GitHub Actions can override this at build time if desired.
    GIT_SHA="custom-build"
)

# ── ESP-IDF project boilerplate ──────────────────────────────────────
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(${PROJECT_NAME})
