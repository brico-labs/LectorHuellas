#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "config/config.h"
#include <ESP8266httpUpdate.h>

const short BUZZER = D1;
WiFiClient wifiClient;
PubSubClient client(wifiClient);
unsigned long lastUpdateTime = 0;

void alarm(uint16 seconds) {
  digitalWrite(BUZZER, HIGH);
  delay(seconds * 1000);
  digitalWrite(BUZZER, LOW);
}

void reconnect() {
  static unsigned long lastConnection = millis();

  if (client.connected() || millis() - lastConnection < 10000)
    return;
  
  lastConnection = millis();

  if (client.connect(String(ESP.getChipId()).c_str(), MQTT_USER, MQTT_PASSWORD)) {
    Serial.println("MQTT connected");
    String topicSiren = String("/cmdn/siren/") + String(ESP.getChipId()) + "/on";
    client.subscribe(topicSiren.c_str());
  } 
  else {
    Serial.println("MQTT Connection failed");
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String topicSiren = String("/cmdn/siren/") + String(ESP.getChipId()) + "/on";

  if (!strcmp(topic, topicSiren.c_str())) {
    Serial.println("Siren activated from MQTT Command");
    char payloadText[3];

    if (length > 2) {
      Serial.println("Mensaje mqtt incorrecto");
      return;
    }

    memcpy(payloadText, payload, length);
    payloadText[length] = '\0';

    Serial.print("Alarma on");
    alarm(atoi(payloadText));
    Serial.print("Alarma off"); 
  }
}

void setup() {
  
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(100);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

    // setup wifi connection
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
    static int i = 0;
    i++;
    if (i>120) // 1 min
      ESP.restart();
    Serial.print(".");
    delay(500);
  }

  Serial.println("Connected to network!");

  client.setServer(MQTT_ADDR, 1883);
  client.setCallback(callback);
  delay(100);

  Serial.println(ESP.getChipId());
}

void loop() {
  // put your main code here, to run repeatedly:

  if (!client.connected()) { // Reconnect to MQTT if necesary
    reconnect();
  }

  client.loop(); // MQTT update

  if (millis() - lastUpdateTime > 60 * 60 * 1000) {
    lastUpdateTime = millis();
    String url = "http://radon.4m1g0.com/update/";
    url += String(ESP.getChipId()) + String(".bin");
    t_httpUpdate_return ret = ESPhttpUpdate.update(url);

    switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        break;

      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        break;

      case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        break;
    }
  }
}