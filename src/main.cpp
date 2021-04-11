#include <Arduino.h>
#include <Wire.h>
#include "DFRobot_SHT20.h"
#include <WiFi.h>
#include <MQTTClient.h> // MQTT Client from Joël Gaehwiler https://github.com/256dpi/arduino-mqtt   keepalive manually to 15s

const char *WiFi_SSID = "YourWiFiSSID";
const char *WiFi_PW = "YourWiFiPassword";
const char *mqtt_broker = "YourMQTTBrokerIP";
const char *mqtt_user = "YourMQTTBrokerUsername";
const char *mqtt_pw = "YourMQTTBrokerPassword";
const char *input_topic = "YourTopic";
String clientId = "ESP32Client-";                 // Necessary for user-pass auth
const int stationID = 1;                          // check stations table in database

unsigned long waitCount = 0; // counter
uint8_t conn_stat = 0;       // Connection status for WiFi and MQTT:
                             //
                             // status |   WiFi   |    MQTT
                             // -------+----------+------------
                             //      0 |   down   |    down
                             //      1 | starting |    down
                             //      2 |    up    |    down
                             //      3 |    up    |  starting
                             //      4 |    up    | finalising
                             //      5 |    up    |     up

unsigned long lastStatus = 0; // counter in example code for conn_stat == 5
unsigned long lastTask = 0;   // counter in example code for conn_stat <> 5

const char *Version = "{\"Version\":\"low_prio_wifi_v2\"}";
const char *Status = "{\"Message\":\"up and running\"}";

WiFiClient espClient;       // TCP client object, uses SSL/TLS
MQTTClient mqttClient(512); // MQTT client object with a buffer size of 512 (depends on your message size)

DFRobot_SHT20 sht20;

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

void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_STA); // config WiFi as client
  // printPins();
  sht20.initSHT20();
  delay(100);
  sht20.checkSHT20();
}

void loop()
{ // with current code runs roughly 400 times per second
  // start of non-blocking connection setup section
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
    Serial.println("WiFi starting, wait : " + String(waitCount));
    waitCount++;
    break;
  case 2: // WiFi up, MQTT down: start MQTT
    Serial.println("WiFi up, MQTT down: start MQTT");
    mqttClient.begin(mqtt_broker, 1883, espClient); //   config MQTT Server, use port 8883 for secure connection
    clientId += String(random(0xffff), HEX);        // Create a random
    mqttClient.connect(clientId.c_str(), mqtt_user, mqtt_pw);
    conn_stat = 3;
    waitCount = 0;
    break;
  case 3: // WiFi up, MQTT starting, do nothing here
    Serial.println("WiFi up, MQTT starting, wait : " + String(waitCount));
    mqttClient.connect(clientId.c_str(), mqtt_user, mqtt_pw);
    waitCount++;
    break;
  case 4: // WiFi up, MQTT up: finish MQTT configuration
    Serial.println("WiFi up, MQTT up: finish MQTT configuration");
    mqttClient.publish(input_topic, Version); // this is necessary if broker is down
    conn_stat = 5;
    break;
  }
  // end of non-blocking connection setup section

  // start section with tasks where WiFi/MQTT is required
  if (conn_stat == 5)
  {
    if (millis() - lastStatus > 10000)
    {
      float hum = sht20.readHumidity();
      float temp = sht20.readTemperature();
      String json = "{\"temperature\":\"" + String(temp) + "\" , \"humidity\":\"" + String(hum) + "\", \"stationID\":\"" + stationID + "\" }";
      char *payload = &json[0]; // converts String to char*
      mqttClient.publish(input_topic, payload);
      mqttClient.loop();     //      give control to MQTT to send message to broker
      lastStatus = millis(); //      remember time of last sent status message
    }
    mqttClient.loop(); // internal household function for MQTT
  }
  // end of section for tasks where WiFi/MQTT are required

  // start section for tasks which should run regardless of WiFi/MQTT
  // if (millis() - lastTask > 1000) {                                 // Print message every second (just as an example)
  //   Serial.println("print this every second");
  //   lastTask = millis();
  // }
  // delay(100);
  // end of section for tasks which should run regardless of WiFi/MQTT
}
