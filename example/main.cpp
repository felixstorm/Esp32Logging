#include <Arduino.h>
#include <Wifi.h>
#include <Esp32LogToSyslog.hpp>

WiFiUDP UdpClient;
Esp32LogToSyslog LogToSyslog(UdpClient);

void setup()
{
    const char buffer[] = "Test string - a little bit longer and containing \n non-printable chars.";

    esp_log_level_set("*", ESP_LOG_INFO);

    Serial.begin(115200);
    Serial.println("\n\n*** Esp32Logging Example Code Starting");
    delay(1000);

    Serial.println("*** Starting WiFi...");
    WiFi.begin("SSID", "Password");
    while(!WiFi.isConnected());
    delay(2000);
    Serial.println("*** WiFi connected.");

    // start syslog
    LogToSyslog.begin(IPAddress(192, 168, 0, 33));
    
    // directly send a message
    if (!LogToSyslog.sendSyslogMessage("Test Message from Esp32LogToSyslog"))
        Serial.println("*** ERROR in sendSyslogMessage()");

    // log error
    ESP_LOGE("tag", "Message ESP_LOGE");
    ESP_LOG_BUFFER_HEXDUMP("TagE", buffer, sizeof(buffer), ESP_LOG_ERROR);

    // log info
    ESP_LOGI("tag", "Message ESP_LOGI");
    ESP_LOG_BUFFER_HEXDUMP("TagI", buffer, sizeof(buffer), ESP_LOG_INFO);

    // allow some time to send log messages
    delay(2000);

    Serial.println("*** Disconnecting WiFi...");
    WiFi.disconnect(true, true);
    while(WiFi.isConnected());
    delay(2000);
    Serial.println("*** WiFi disconnected.");
    
    // log info without WiFi...
    ESP_LOGE("tag", "A message  without Wifi");
    ESP_LOGI("tag", "Another message  without Wifi");
}

void loop()
{
}
