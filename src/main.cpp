#include <Arduino.h>
#include <Wire.h>
#include "DFRobot_SHT20.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <Measurement.h>
#include <Connection.h>


int stationID = 3; // check stations table in database
unsigned long readInterval = 2000;
unsigned long uploadInterval = 10000;

unsigned long currentMillis, lastReadMillis, lastUploadMillis;

DFRobot_SHT20 sht20;
Measurement temperature, humidity;

const int16_t messageSize = 256;
Connection connection(messageSize);
StaticJsonDocument<messageSize> jsonDoc;
char payload[messageSize];

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

void waitForNextMinute()
{
  while (!connection.statusOK())
    ;
  timeClient.begin();
  while (!timeClient.update())
  {
    timeClient.forceUpdate();
  }
  while (timeClient.getSeconds() != 0)
    ;
}

void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_STA); // config WiFi as client
  // printPins();
  sht20.initSHT20();
  delay(100);
  sht20.checkSHT20();
  // waitForNextMinute();
  lastUploadMillis = millis();
}

void updateJson()
{
  jsonDoc["stationID"] = stationID;
  jsonDoc["count"] = temperature.count();
  jsonDoc["Tmin"] = temperature.min();
  jsonDoc["Tmax"] = temperature.max();
  jsonDoc["Tmean"] = temperature.mean();
  jsonDoc["Tstdev"] = temperature.stdev();
  jsonDoc["RHmin"] = humidity.min();
  jsonDoc["RHmax"] = humidity.max();
  jsonDoc["RHmean"] = humidity.mean();
  jsonDoc["RHstdev"] = humidity.stdev();
  jsonDoc["freeHeap"] = ESP.getFreeHeap();
}

void loop()
{
  if (connection.statusOK())
  {
    currentMillis = millis();
    if (currentMillis - lastReadMillis >= readInterval)
    {
      lastReadMillis = currentMillis;
      temperature.sample(sht20.readTemperature());
      humidity.sample(sht20.readHumidity());
    }
    currentMillis = millis();
    if (currentMillis - lastUploadMillis >= uploadInterval)
    {
      lastUploadMillis = currentMillis;
      updateJson();
      temperature.reset();
      humidity.reset();
      serializeJson(jsonDoc, payload);
      connection.upload(payload);
    }
    connection.check();
  }
}
