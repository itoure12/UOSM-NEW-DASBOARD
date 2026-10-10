// portenta_bridge.ino — Wi-Fi bridge for the dashboard (Arduino Portenta H7).
//
// Subscribes to the MQTT topics "uosm/#" over Wi-Fi and forwards each message
// to the STM32F769 dashboard on Serial1 (pins TX/RX), one text line per
// message: "topic value\n" (e.g. "uosm/battery/voltage 47.02").
//
// Wiring (3.3 V on both sides):
//   Portenta TX -> STM32 Arduino header D0 (PC7, USART6_RX)
//   Portenta RX -> STM32 Arduino header D1 (PC6, USART6_TX)
//   GND         -> GND
//
// Setup:
//   - Arduino IDE: board package "Arduino Mbed OS Portenta Boards",
//     library "ArduinoMqttClient".
//   - Copy arduino_secrets.h.example to arduino_secrets.h and fill it in.
//     arduino_secrets.h is ignored by git: never commit real credentials.
//   - Wi-Fi must be 2.4 GHz, WPA2-Personal (iPhone hotspot: enable
//     "Maximize Compatibility").
//   - Board: "Arduino Portenta H7 (M7 core)". Use Upload, not Debug.
#include <WiFi.h>
#include <ArduinoMqttClient.h>

#include "arduino_secrets.h"

const char* WIFI_SSID = SECRET_WIFI_SSID;
const char* WIFI_PASS = SECRET_WIFI_PASS;
const char* BROKER    = SECRET_BROKER_IP;   // IP of the PC running Mosquitto, on this Wi-Fi
const int   PORT      = 1883;

WiFiClient wifi;
MqttClient mqtt(wifi);

// Called for each message received from the broker
void onMessage(int size) {
  String topic = mqtt.messageTopic();
  String value;
  while (mqtt.available()) {
    value += (char)mqtt.read();
  }

  Serial1.print(topic);       // to the STM32
  Serial1.print(' ');
  Serial1.println(value);

  Serial.print("MQTT -> STM32 : ");   // to the Serial Monitor
  Serial.print(topic);
  Serial.print(' ');
  Serial.println(value);
}

void connectWifi() {
  Serial.print("Connecting to Wi-Fi ");
  Serial.println(WIFI_SSID);
  while (WiFi.begin(WIFI_SSID, WIFI_PASS) != WL_CONNECTED) {
    Serial.println("  failed, retrying in 2 s");
    delay(2000);
  }
  Serial.print("Wi-Fi OK, Portenta IP: ");
  Serial.println(WiFi.localIP());
}

void connectMqtt() {
  Serial.print("Connecting to broker ");
  Serial.println(BROKER);
  mqtt.setId("portenta-bridge");
  while (!mqtt.connect(BROKER, PORT)) {
    // -2 = the broker cannot be reached: wrong IP, Windows network profile not
    // "Private", or Mosquitto started without a "listener 1883" config
    Serial.print("  failed (code ");
    Serial.print(mqtt.connectError());
    Serial.println("), retrying in 2 s");
    delay(2000);
  }
  mqtt.onMessage(onMessage);
  mqtt.subscribe("uosm/#");
  Serial.println("MQTT OK, subscribed to uosm/#");
}

void setup() {
  Serial.begin(115200);    // USB, Serial Monitor
  Serial1.begin(115200);   // pins TX/RX, to the STM32
  delay(1000);
  connectWifi();
  connectMqtt();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWifi();
  if (!mqtt.connected())             connectMqtt();
  mqtt.poll();   // processes received messages (calls onMessage)
}
