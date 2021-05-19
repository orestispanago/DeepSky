#include <Arduino.h>
#include <Wire.h>
#include "DFRobot_SHT20.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Measurement.h>
#include <Connection.h>
#include <utils.h>
#include <Timing.h>

unsigned long readInterval = 1000;
unsigned long uploadInterval = 10000;

unsigned long currentMillis, lastReadMillis, lastUploadMillis;

DFRobot_SHT20 sht20;
Measurement temperature, humidity;

int stationId = intFromUserName(mqtt_user);

void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_STA); // config WiFi as client
  // printPins();
  sht20.initSHT20();
  delay(100);
  sht20.checkSHT20();
  waitForNextMinute();
  lastUploadMillis = millis();
}

void updateJson()
{
  jsonDoc["stationID"] = stationId;
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

void sampleAll()
{
  temperature.sample(sht20.readTemperature());
  humidity.sample(sht20.readHumidity());
}

void uploadAll()
{
  updateJson();
  temperature.reset();
  humidity.reset();
  upload();
}

void loop()
{
  if (statusOK())
  {
    currentMillis = millis();
    if (currentMillis - lastReadMillis >= readInterval)
    {
      lastReadMillis = currentMillis;
      sampleAll();
    }
    currentMillis = millis();
    if (currentMillis - lastUploadMillis >= uploadInterval)
    {
      lastUploadMillis = currentMillis;
      uploadAll();
    }
    check();
  }
}
