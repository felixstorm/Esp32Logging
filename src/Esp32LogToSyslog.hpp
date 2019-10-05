#pragma once

#include <Arduino.h>
#include <Esp32Logging.hpp>

#define ESP32LOGTOSYSLOG_QUEUE_SIZE 1024


class Esp32LogToSyslog
{
public:
    Esp32LogToSyslog(UDP &client, const char* deviceHostname = "ESP32");
    void begin(const char* server, const char* deviceHostname = nullptr, uint16_t port = 514);
    void begin(IPAddress ip, const char* deviceHostname = nullptr, uint16_t port = 514);
    bool sendSyslogMessage(const char *message, int pri = -1);

private:
    UDP* _client;
    const char* _deviceHostname;
    const char* _server;
    IPAddress _ip;
    uint16_t _port;
    bool _packetStarted = false;

    static QueueHandle_t xCharQueue;
    static void ets_putc_replacement(char c);
    static int vprintf_replacement(const char *fmt, va_list args);
    static void logOutputTask(void *parameter);

    void begin();
    void sendCharToSyslog(char c);
    bool beginSyslogPacket(int pri = -1);
    bool endSyslogPacket();
};
