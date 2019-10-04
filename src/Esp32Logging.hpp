#pragma once

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
