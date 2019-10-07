#include "Esp32ExtendedLogging.hpp"
#include <WiFi.h>

QueueHandle_t Esp32ExtendedLogging::xCharQueue = xQueueCreate(ESP32LOGTOSYSLOG_QUEUE_SIZE, sizeof(char));
bool Esp32ExtendedLogging::queueUartOutputFlag = false;

Esp32ExtendedLogging &Esp32ExtendedLogging::queueUartOutput(bool queueUartOutput)
{
    queueUartOutputFlag = queueUartOutput; // static
    return *this;
}

Esp32ExtendedLogging &Esp32ExtendedLogging::doSyslog(UDP &client, const char *server, uint16_t port)
{
    this->_client = &client;

    IPAddress ipAddr;
    if (ipAddr.fromString(server))
        this->_ip = ipAddr;
    else
        this->_server = server;

    this->_port = port;

    _doSyslog = true;

    return *this;
}

Esp32ExtendedLogging &Esp32ExtendedLogging::deviceHostname(const char *deviceHostname)
{
    this->_syslogHostname = deviceHostname;
    return *this;
}

Esp32ExtendedLogging &Esp32ExtendedLogging::begin()
{
    if (!_beginCalled)
    {
        xTaskCreate(logOutputTask, "Esp32LogToSyslogLogOutputTask", 2048, this, 1, NULL);

        esp_log_set_vprintf(vprintf_replacement);
        ets_install_putc1((void (*)(char)) & ets_putc_replacement);
    }

    this->_beginCalled = true;

    return *this;
}

void IRAM_ATTR Esp32ExtendedLogging::ets_putc_replacement(char c)
{
    // send to serial (as usual)
    if (!queueUartOutputFlag)
        ets_write_char_uart(c);

    // also send to queue to be sysloged
    if (xPortInIsrContext())
    {
        // Sample code from FreeRTOS asks to call taskYIELD on xHigherPriorityTaskWoken,
        // but this leads to WDT timeouts - so we ignore it for now.
        xQueueSendToBackFromISR(xCharQueue, &c, nullptr);
    }
    else
    {
        xQueueSendToBack(xCharQueue, &c, 0); // do not block at all
    }
}

int Esp32ExtendedLogging::vprintf_replacement(const char *fmt, va_list args)
{
    const size_t initialLen = strlen(fmt);
    char *message = new char[initialLen + 1];
    size_t len = vsnprintf(message, initialLen + 1, fmt, args);
    if (len > initialLen)
    {
        delete[] message;
        message = new char[len + 1];
        vsnprintf(message, len + 1, fmt, args);
    }

    // send to serial (as usual)
    if (!queueUartOutputFlag)
        printf(message);

    // also send to queue to be sysloged (not to be used from ISRs anyway)
    char *bufferForQueue = message;
    while (*bufferForQueue)
        xQueueSendToBack(xCharQueue, bufferForQueue++, 0); // do not block at all

    delete[] message;
    return true;
}

void Esp32ExtendedLogging::logOutputTask(void *parameter)
{
    Esp32ExtendedLogging *thisClass = (Esp32ExtendedLogging *)parameter;
    for (;;)
    {
        char c;
        if (xQueueReceive(xCharQueue, &c, (TickType_t)10))
        {
            if (queueUartOutputFlag)
                Serial.print(c);
            if (thisClass->_doSyslog)
                thisClass->sendCharToSyslog(c);
        }
    }
}

void Esp32ExtendedLogging::sendCharToSyslog(char c)
{
    static const ulong delayOnError = 2000;
    static ulong lastError = 0;

    // Skip trying to syslog for a few seconds to avoid error message clogging up queue
    if (lastError && millis() - lastError < delayOnError)
        return;

    // No need to try without network
    if (!WiFi.isConnected())
        return;

    do
    {
        if (c != '\n')
        {
            if (!_packetStarted)
            {
                if (!beginSyslogPacket())
                    break;
            }
            if (!this->_client->print(c))
                break;
        }
        else
        {
            if (_packetStarted)
            {
                if (!endSyslogPacket())
                    break;
            }
        }

        return;

    } while (0);

    lastError = millis();
    _packetStarted = false;
    ESP_LOGE("Esp32ExtendedLogging", "Error sending syslog message, pausing for %d seconds...", (uint)delayOnError / 1000);
    return;
}

// based on https://github.com/arcao/Syslog
bool Esp32ExtendedLogging::beginSyslogPacket(int pri)
{
    if ((this->_server == NULL && this->_ip == INADDR_NONE) || this->_port == 0)
        return false;

    if (pri < 0)
        pri = 1 << 3 | 6; // user, informational

    if (!(this->_server != NULL ? this->_client->beginPacket(this->_server.c_str(), this->_port) : this->_client->beginPacket(this->_ip, this->_port)))
        return false;

    // IETF Doc: https://tools.ietf.org/html/rfc5424
    // BSD Doc: https://tools.ietf.org/html/rfc3164
    this->_client->print('<');
    this->_client->print(pri);
    this->_client->print('>');
    this->_client->print(this->_syslogHostname);
    this->_client->print(' ');

    _packetStarted = true;

    return true;
}

bool Esp32ExtendedLogging::endSyslogPacket()
{
    if (!_packetStarted)
        return false;

    bool result = this->_client->endPacket();

    _packetStarted = false;
    return result;
}

bool Esp32ExtendedLogging::sendSyslogMessage(const char *message, int pri)
{
    if (!beginSyslogPacket())
        return false;

    this->_client->print(message);

    if (!endSyslogPacket())
        return false;

    return true;
}
