#pragma once

#include <Esp32Logging.hpp>

#define ESP32LOGTOSYSLOG_QUEUE_SIZE 4096


class Esp32ExtendedLogging
{
public:
    Esp32ExtendedLogging& queueUartOutput(bool queueUartOutput);
    Esp32ExtendedLogging& doSyslog(UDP &client, const char *server, uint16_t port = 514);
    Esp32ExtendedLogging& deviceHostname(const char *deviceHostname);
    Esp32ExtendedLogging& begin();
    bool sendSyslogMessage(const char *message, int pri = -1);

private:
    static bool queueUartOutputFlag;
    bool _doSyslog = false;
    UDP* _client;
    String _server;
    IPAddress _ip;
    uint16_t _port;
    String _syslogHostname;
    bool _beginCalled = false;
    bool _packetStarted = false;

    static QueueHandle_t xCharQueue;
    static void ets_putc_replacement(char c);
    static int vprintf_replacement(const char *fmt, va_list args);
    static void logOutputTask(void *parameter);

    void sendCharToSyslog(char c);
    bool beginSyslogPacket(int pri = -1);
    bool endSyslogPacket();
};
