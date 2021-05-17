#include <Arduino.h>
#include <Wire.h>
#include "DFRobot_SHT20.h"
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <MQTTClient.h> // MQTT Client from Joël Gaehwiler https://github.com/256dpi/arduino-mqtt   keepalive manually to 15s
#include <ArduinoJson.h>
#include <Measurement.h>


const char* WiFi_SSID = "YourWiFiSSID";
const char* WiFi_PW = "YourWiFiPassword";
const char* mqtt_broker = "YourMQTTBrokerIP";
const char* mqtt_user = "YourMQTTBrokerUsername";
const char* mqtt_pw = "YourMQTTBrokerPassword";
const char* input_topic = "YourTopic";

int stationID = 3; // check stations table in database
unsigned long readInterval = 3000;
unsigned long uploadInterval = 60000;

unsigned long currentMillis, lastReadMillis, lastUploadMillis;

DFRobot_SHT20 sht20;
Measurement temperature, humidity;

String clientId;

unsigned long waitCount;
uint8_t conn_stat;

WiFiClient espClient;
const int16_t messageSize = 256;
MQTTClient mqttClient(messageSize);

StaticJsonDocument<messageSize> jsonDoc;
char payload[messageSize];

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

void printPins()
{
  Serial.println();
  Serial.println("======================");
  Serial.println("Hardware SPI GPIO pins");
  Serial.print("MOSI: \t");
  Serial.println(MOSI);
  Serial.print("MISO: \t");
  Serial.println(MISO);
  Serial.print("SCK: \t");
  Serial.println(SCK);
  Serial.print("SS: \t");
  Serial.println(SS);
  Serial.println("======================");
  Serial.println("Default I2C GPIO pins");
  Serial.print("SDA: \t");
  Serial.println(SDA);
  Serial.print("SCL: \t");
  Serial.println(SCL);
  Serial.println();
}

boolean connected()
{
  // Connection status for WiFi and MQTT:
  //
  // status |   WiFi   |    MQTT
  // -------+----------+------------
  //      0 |   down   |    down
  //      1 | starting |    down
  //      2 |    up    |    down
  //      3 |    up    |  starting
  //      4 |    up    | finalising
  //      5 |    up    |     up
  if ((WiFi.status() != WL_CONNECTED) && (conn_stat != 1))
  {
    conn_stat = 0;
  }
  if ((WiFi.status() == WL_CONNECTED) && !mqttClient.connected() && (conn_stat != 3))
  {
    conn_stat = 2;
  }
  if ((WiFi.status() == WL_CONNECTED) && mqttClient.connected() && (conn_stat != 5))
  {
    conn_stat = 4;
  }
  switch (conn_stat)
  {
  case 0: // MQTT and WiFi down: start WiFi
    Serial.println("MQTT and WiFi down: start WiFi");
    WiFi.begin(WiFi_SSID, WiFi_PW);
    conn_stat = 1;
    break;
  case 1: // WiFi starting, do nothing here
    Serial.print("WiFi starting, wait : ");
    Serial.println(waitCount);
    waitCount++;
    break;
  case 2: // WiFi up, MQTT down: start MQTT
    Serial.println("WiFi up, MQTT down: start MQTT");
    mqttClient.begin(mqtt_broker, 1883, espClient);          //   config MQTT Server, use port 8883 for secure connection
    clientId = "ESP32Client-" + String(random(0xffff), HEX); // Create a random client ID
    mqttClient.connect(clientId.c_str(), mqtt_user, mqtt_pw);
    conn_stat = 3;
    waitCount = 0;
    break;
  case 3: // WiFi up, MQTT starting, do nothing here
    Serial.print("WiFi up, MQTT starting, wait : ");
    Serial.println(waitCount);
    mqttClient.connect(clientId.c_str(), mqtt_user, mqtt_pw);
    waitCount++;
    break;
  case 4: // WiFi up, MQTT up: finish MQTT configuration
    Serial.println("WiFi up, MQTT up: finish MQTT configuration");
    mqttClient.publish(input_topic, "{\"Status\":\"up and running!\"}");
    conn_stat = 5;
    break;
  }
  return conn_stat == 5;
}

void waitForNextMinute()
{
  while (!connected())
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
  waitForNextMinute();
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
  if (connected())
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
      mqttClient.publish(input_topic, payload);
      mqttClient.loop(); //      give control to MQTT to send message to broker
    }
    mqttClient.loop();
  }
}
