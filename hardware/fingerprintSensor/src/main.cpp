#include "FingerprintSensor/FingerprintSensor.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "config/config.h"
#include "StatusLeds/StatusLeds.h"
#include <ESP8266httpUpdate.h>

const short DOOR = D2;
const short BUZZER = D5;
unsigned long lastUpdateTime = 0;

FingerprintSensor fingerSensor;
WiFiClient wifiClient;
PubSubClient client(wifiClient);

void open() {
  digitalWrite(DOOR, HIGH);
  digitalWrite(BUZZER, HIGH);
  digitalWrite(GREEN_LED, HIGH);
  delay(1000);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(BUZZER, LOW);
  digitalWrite(DOOR, LOW);
}

void reconnect() {
  static unsigned long lastConnection = millis();

  if (client.connected() || millis() - lastConnection < 10000)
    return;
  
  lastConnection = millis();

  if (client.connect(String(ESP.getChipId()).c_str(), MQTT_USER, MQTT_PASSWORD)) {
    Serial.println("MQTT connected");
    String topicOpen = String("/cmdn/lock/") + String(ESP.getChipId()) + "/open";
    String topicAdd = String("/cmdn/lock/") + String(ESP.getChipId()) + "/add";
    String topicRemove = String("/cmdn/lock/") + String(ESP.getChipId()) + "/remove";
    client.subscribe(topicOpen.c_str());
    client.subscribe(topicAdd.c_str());
    client.subscribe(topicRemove.c_str());
  } 
  else {
    Serial.println("MQTT Connection failed");
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String topicOpen = String("/cmdn/lock/") + ESP.getChipId() + "/open";
  String topicAdd = String("/cmdn/lock/") + ESP.getChipId() + "/add";
  String topicRemove = String("/cmdn/lock/") + ESP.getChipId() + "/remove";
  if (!strcmp(topic, topicOpen.c_str())) {
    Serial.println("Opening from MQTT Command");
    open();
  } 
  else if (!strcmp(topic, topicAdd.c_str())) {
    Serial.println("Adding new fingerprint to the system");
    int8_t id = fingerSensor.fingerprintEnroll();
    Serial.print("ID: ");
    Serial.println(id);
    String topic = String("/stat/lock/") + String(ESP.getChipId()) + "/enrollingId";
    char payload[4] = "";
    itoa(id,payload, 10);
    Serial.println(id);
    long init_time = millis();
    while (!client.connected()) {
      reconnect();
      delay(200);
      if (millis() - init_time) {
        break;
      }
    }
    if (client.connected()) {
      client.publish(topic.c_str(), payload);
    } 
    else {
      // TODO: do something in caso of fail
    }
    
  }
  else if (!strcmp(topic, topicRemove.c_str())) {
    char payloadText[3];

    if (length > 2) {
      Serial.println("Mensaje mqtt incorrecto");
      return;
    }

    memcpy(payloadText, payload, length);
    payloadText[length] = '\0';
    if (!fingerSensor.deleteFinger(atoi(payloadText))) {
      Serial.print("Huella eliminada: ");
      Serial.println(payloadText);
    } 
    else {
      Serial.print("Hubo un error al eliminar la huella: ");
      Serial.println(payloadText);
    }
  }

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}



void setup()
{
  Serial.begin(115200);
  delay(100);
  fingerSensor.begin();
  StatusLeds::begin();
  pinMode(D5, INPUT_PULLUP);
  pinMode(DOOR, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  // setup wifi connection
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }

  Serial.println("Connected to network!");

  client.setServer(MQTT_ADDR, 1883);
  client.setCallback(callback);
  delay(100);
}

void loop()
{
  int code = fingerSensor.getFingerprintID();

  if (code >= 0) { // Fingerprint found
    if (client.connected()) {
      String topic = String("/tele/lock/") + String(ESP.getChipId()) + "/open";
      char payload[3] = "";
      itoa(code,payload, 10);
      client.publish(topic.c_str(), payload);
    }
    else {
      // TODO: we have to persist this and send log when the connection is available
    }

    open();
  }
  else if (code == -2) { // Fingeprint not found
    if (client.connected()) {
      String topic = String("/tele/lock/") + String(ESP.getChipId()) + "/denied";
      char payload[1] = "";
      client.publish(topic.c_str(), payload);
    }
    else {
      // TODO: we have to persist this and send log when the connection is available
    }
    StatusLeds::on(RED_LED);
    for (int i = 0; i < 4; i++) {
      digitalWrite(BUZZER, HIGH);
      delay(100);
      digitalWrite(BUZZER, LOW);
      delay(100);
    }
    delay(200);
    StatusLeds::off(RED_LED);
  }

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

