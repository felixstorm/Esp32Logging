#pragma once

// need to include this up here as it would conflict with our redefinitions if included later
#include <Arduino.h>

/*
*********************************************************************************************
*** Logging must be enabled BOTH in platformio.ini AND in code using esp_log_level_set!!! ***
*********************************************************************************************
platformio.ini: build_flags = -D CORE_DEBUG_LEVEL=ARDUHAL_LOG_LEVEL_INFO -D LOG_LOCAL_LEVEL=ESP_LOG_INFO
setup(): esp_log_level_set("*", ESP_LOG_INFO);

* Use normal ESP32-IDF logging functions like ESP_LOGI, ESP_LOG_BUFFER_HEXDUMP etc.
* ESP_LOGx are redefined to include filename, line and function if used below this file,
  but not when used from SDK. Also ESP_LOG_BUFFER_HEXDUMP handles output inside compiled code,
  so it cannot benefit either.
*/

// Revert redefines of Arduino-ESP32 esp32-hal-log.h to be able to use tags for logging again
#undef ESP_LOGE
#undef ESP_LOGW
#undef ESP_LOGI
#undef ESP_LOGD
#undef ESP_LOGV
#undef LOG_FORMAT
#undef ESP_LOG_LEVEL

#define ESP_LOGE( tag, format, ... ) ESP_LOG_LEVEL_LOCAL(ESP_LOG_ERROR,   tag, format, ##__VA_ARGS__)
#define ESP_LOGW( tag, format, ... ) ESP_LOG_LEVEL_LOCAL(ESP_LOG_WARN,    tag, format, ##__VA_ARGS__)
#define ESP_LOGI( tag, format, ... ) ESP_LOG_LEVEL_LOCAL(ESP_LOG_INFO,    tag, format, ##__VA_ARGS__)
#define ESP_LOGD( tag, format, ... ) ESP_LOG_LEVEL_LOCAL(ESP_LOG_DEBUG,   tag, format, ##__VA_ARGS__)
#define ESP_LOGV( tag, format, ... ) ESP_LOG_LEVEL_LOCAL(ESP_LOG_VERBOSE, tag, format, ##__VA_ARGS__)

#define LOG_FORMAT(letter, tag, format)  #letter " (%d) %s: [%s:%u] %s(): " format "\n", esp_log_timestamp(), tag, pathToFileName(__FILE__), __LINE__, __FUNCTION__

#define ESP_LOG_LEVEL(level, tag, format, ...) do {                     \
        if (level==ESP_LOG_ERROR )          { esp_log_write(ESP_LOG_ERROR,      tag, LOG_FORMAT(E, tag, format), ##__VA_ARGS__); } \
        else if (level==ESP_LOG_WARN )      { esp_log_write(ESP_LOG_WARN,       tag, LOG_FORMAT(W, tag, format), ##__VA_ARGS__); } \
        else if (level==ESP_LOG_DEBUG )     { esp_log_write(ESP_LOG_DEBUG,      tag, LOG_FORMAT(D, tag, format), ##__VA_ARGS__); } \
        else if (level==ESP_LOG_VERBOSE )   { esp_log_write(ESP_LOG_VERBOSE,    tag, LOG_FORMAT(V, tag, format), ##__VA_ARGS__); } \
        else                                { esp_log_write(ESP_LOG_INFO,       tag, LOG_FORMAT(I, tag, format), ##__VA_ARGS__); } \
    } while(0)


#define ESP_ISR_LOGE( tag, format, ... ) ESP_LOG_ISR_IMPL(tag, format, ESP_LOG_ERROR,   E, ##__VA_ARGS__)
#define ESP_ISR_LOGW( tag, format, ... ) ESP_LOG_ISR_IMPL(tag, format, ESP_LOG_WARN,    W, ##__VA_ARGS__)
#define ESP_ISR_LOGI( tag, format, ... ) ESP_LOG_ISR_IMPL(tag, format, ESP_LOG_INFO,    I, ##__VA_ARGS__)
#define ESP_ISR_LOGD( tag, format, ... ) ESP_LOG_ISR_IMPL(tag, format, ESP_LOG_DEBUG,   D, ##__VA_ARGS__)
#define ESP_ISR_LOGV( tag, format, ... ) ESP_LOG_ISR_IMPL(tag, format, ESP_LOG_VERBOSE, V, ##__VA_ARGS__)

#define ESP_LOG_ISR_IMPL(tag, format, log_level, log_tag_letter, ...) do {                         \
        if (LOG_LOCAL_LEVEL >= log_level) {                                                          \
            ets_printf(LOG_FORMAT(log_tag_letter, tag, format), ##__VA_ARGS__); \
        }} while(0)


#define ESP_LOG_SYSINFO(full) do {                                                                                                                                   \
    if (full) {                                                                                                                                                      \
        esp_chip_info_t info;                                                                                                                                        \
        esp_chip_info(&info);                                                                                                                                        \
        ESP_LOGI("SysInfo", "Chip info: model:%s, cores:%d, feature:%s%s%s%s%d MB, revision number:%d, IDF Version:%s",                                              \
            info.model == CHIP_ESP32 ? "ESP32" : "Unknow", info.cores,                                                                                               \
            info.features & CHIP_FEATURE_WIFI_BGN ? "/802.11bgn" : "", info.features & CHIP_FEATURE_BLE ? "/BLE" : "", info.features & CHIP_FEATURE_BT ? "/BT" : "", \
            info.features & CHIP_FEATURE_EMB_FLASH ? "/Embedded-Flash:" : "/External-Flash:", spi_flash_get_chip_size() / (1024 * 1024), info.revision,              \
            esp_get_idf_version());                                                                                                                                  \
    }                                                                                                                                                                \
    ESP_LOGI("SysInfo", "Current free heap size: %u, min free heap size: %u", esp_get_free_heap_size(), heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT));        \
} while(0)
