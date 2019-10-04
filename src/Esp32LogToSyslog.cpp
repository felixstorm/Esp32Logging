#include "Esp32LogToSyslog.hpp"

QueueHandle_t Esp32LogToSyslog::xCharQueue = xQueueCreate(ESP32LOGTOSYSLOG_QUEUE_SIZE, sizeof(char));

Esp32LogToSyslog::Esp32LogToSyslog(UDP &client, const char *deviceHostname)
{
    this->_client = &client;
    this->_deviceHostname = deviceHostname;
}

void Esp32LogToSyslog::begin(const char *server, uint16_t port)
{
    this->_server = server;
    this->_port = port;
    begin();
}

void Esp32LogToSyslog::begin(IPAddress ip, uint16_t port)
{
    this->_ip = ip;
    this->_port = port;
    begin();
}

void Esp32LogToSyslog::begin()
{
    xTaskCreate(logOutputTask, "Esp32LogToSyslogLogOutputTask", 10000, this, 1, NULL);

    esp_log_set_vprintf(vprintf_replacement);
    ets_install_putc1((void (*)(char)) & ets_putc_replacement);
}

void IRAM_ATTR Esp32LogToSyslog::ets_putc_replacement(char c)
{
    // TBD enable (but currently it crashes...)
    // // send to serial (as usual)
    // static const int kUartTxFifoCntS = 16;  // from UART_TXFIFO_CNT_S in framework-arduinoespressif32/tools/sdk/include/soc/soc/uart_reg.h
    // while (((ESP_REG(0x01C + DR_REG_UART_BASE) >> kUartTxFifoCntS) & 0x7F) == 0x7F)
    //     ;
    // ESP_REG(DR_REG_UART_BASE) = c;

    // also send to queue to be sysloged
    if (xPortInIsrContext())
    {
        // Based on sample code from FreeRTOS
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendToBackFromISR(xCharQueue, &c, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken)
            taskYIELD();
    }
    else
    {
        xQueueSendToBack(xCharQueue, &c, 0); // do not block at all
    }
}

int Esp32LogToSyslog::vprintf_replacement(const char *fmt, va_list args)
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

    // TBD enable (currently problems in ets_putc_replacement)
    // // send to serial (as usual)
    // printf(message);

    // also send to queue to be sysloged
    char *buffer = message;
    while (*buffer)
        xQueueSendToBack(xCharQueue, buffer++, 0); // do not block at all

    delete[] message;
    return true;
}

void Esp32LogToSyslog::logOutputTask(void *parameter)
{
    Esp32LogToSyslog *thisClass = (Esp32LogToSyslog *)parameter;
    for (;;)
    {
        char c;
        if (xQueueReceive(xCharQueue, &c, (TickType_t)10))
        {
            thisClass->sendCharToSyslog(c);
            // TBD move to upper layers
            Serial.print(c);
        }
    }
}

void Esp32LogToSyslog::sendCharToSyslog(char c)
{
    static const ulong delayOnError = 2000;
    static ulong lastError = 0;

    // Skip trying to syslog for a few seconds to avoid error message clogging up queue
    if (lastError && millis() - lastError < delayOnError)
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
    ESP_LOGE("Esp32LogToSyslog", "Error sending syslog message, pausing for %d seconds...", (uint)delayOnError / 1000);
    return;
}

// based on https://github.com/arcao/Syslog
bool Esp32LogToSyslog::beginSyslogPacket(int pri)
{
    if ((this->_server == NULL && this->_ip == INADDR_NONE) || this->_port == 0)
        return false;

    if (pri < 0)
        pri = 1 << 3 | 6; // user, informational

    if (!(this->_server != NULL ? this->_client->beginPacket(this->_server, this->_port) : this->_client->beginPacket(this->_ip, this->_port)))
        return false;

    // IETF Doc: https://tools.ietf.org/html/rfc5424
    // BSD Doc: https://tools.ietf.org/html/rfc3164
    this->_client->print('<');
    this->_client->print(pri);
    this->_client->print('>');
    this->_client->print(this->_deviceHostname);
    this->_client->print(' ');

    _packetStarted = true;

    return true;
}

bool Esp32LogToSyslog::endSyslogPacket()
{
    if (!_packetStarted)
        return false;

    bool result = this->_client->endPacket();

    _packetStarted = false;
    return result;
}

bool Esp32LogToSyslog::sendSyslogMessage(const char *message, int pri)
{
    if (!beginSyslogPacket())
        return false;

    this->_client->print(message);

    if (!endSyslogPacket())
        return false;

    return true;
}
