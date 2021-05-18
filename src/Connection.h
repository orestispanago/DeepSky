#ifndef Connection_h
#define Connection_h

#include "Arduino.h"
#include <WiFi.h>
#include <MQTTClient.h> // MQTT Client from Joël Gaehwiler https://github.com/256dpi/arduino-mqtt   keepalive manually to 15s


class Connection
{
public:
    Connection(int16_t messageSize);
    boolean statusOK();
    void upload(char payload[]);
    void check();

private:
    MQTTClient mqttClient;
    uint8_t status;
    unsigned long waitCount;
    WiFiClient espClient;
    String clientId;
};

#endif