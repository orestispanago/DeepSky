#include <Arduino.h>
#include <Wire.h>
#include "DFRobot_SHT20.h"

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

#include <WiFi.h>       // needed for the WiFi communication
#include <MQTTClient.h> // MQTT Client from Joël Gaehwiler https://github.com/256dpi/arduino-mqtt   keepalive manually to 15s

String WiFi_SSID = "YourWiFiSSID";           // change according your setup : SSID and password for the WiFi network
String WiFi_PW = "YourWiFiPassword";         //    "
String mqtt_broker = "YourMQTTBrokerIP";     // change according your setup : IP Adress or FQDN of your MQTT broker
String mqtt_user = "YourMQTTBrokerUsername"; // change according your setup : username and password for authenticated broker access
String mqtt_pw = "YourMQTTBrokerPassword";   //    "
String input_topic = "YourTopic";            // change according your setup : MQTT topic for messages from device to broker

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

void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_STA); // config WiFi as client
  Serial.begin(115200);
  // printPins();
  sht20.initSHT20(); // Init SHT20 Sensor
  delay(100);
  sht20.checkSHT20(); // Check SHT20 Sensor
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
    WiFi.begin(WiFi_SSID.c_str(), WiFi_PW.c_str());
    conn_stat = 1;
    break;
  case 1: // WiFi starting, do nothing here
    Serial.println("WiFi starting, wait : " + String(waitCount));
    waitCount++;
    break;
  case 2: // WiFi up, MQTT down: start MQTT
    Serial.println("WiFi up, MQTT down: start MQTT");
    mqttClient.begin(mqtt_broker.c_str(), 1883, espClient); //   config MQTT Server, use port 8883 for secure connection
    mqttClient.connect(mqtt_user.c_str(), mqtt_pw.c_str());
    conn_stat = 3;
    waitCount = 0;
    break;
  case 3: // WiFi up, MQTT starting, do nothing here
    Serial.println("WiFi up, MQTT starting, wait : " + String(waitCount));
    waitCount++;
    break;
  case 4: // WiFi up, MQTT up: finish MQTT configuration
    Serial.println("WiFi up, MQTT up: finish MQTT configuration");
    //mqttClient.subscribe(output_topic);
    mqttClient.publish(input_topic, Version);
    conn_stat = 5;
    break;
  }
  // end of non-blocking connection setup section

  // start section with tasks where WiFi/MQTT is required
  if (conn_stat == 5)
  {
    if (millis() - lastStatus > 10000)
    {                                       // Start send status every 10 sec (just as an example)
                                            // Serial.println(Status);
      float hum = sht20.readHumidity();    // Read Humidity
      float temp = sht20.readTemperature(); // Read Temperature
      String json = "{\"temperature\":\"" + String(temp) + "\" , \"humidity\":\"" + String(hum) + "\"}";
      char *payload = &json[0]; // converts String to char*
      mqttClient.publish(input_topic, payload);
      // mqttClient.publish(input_topic, Status); //      send status to broker
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
