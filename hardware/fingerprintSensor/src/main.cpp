#include "FingerprintSensor/FingerprintSensor.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "config/password.h"
#include "StatusLeds/StatusLeds.h"

const short DOOR = D2;
const short BUZZER = D5;

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

void callback(char* topic, byte* payload, unsigned int length) {
  String topicOpen = String("lock/") + ID + "/cmd/open";
  String topicAdd = String("lock/") + ID + "/cmd/add";
  if (!strcmp(topic, topicOpen.c_str())) {
    Serial.println("Opening from MQTT Command");
    open();
  } 
  else if (!strcmp(topic, topicAdd.c_str())) {
    Serial.println("Adding new fingerprint to the system");
    fingerSensor.fingerprintEnroll();
  }

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  static unsigned long lastConnection = millis();

  if (client.connected() || millis() - lastConnection < 10000)
    return;
  
  lastConnection = millis();

  if (client.connect(ID, MQTT_USER, MQTT_PASSWORD)) {
    Serial.println("MQTT connected");
    String topicOpen = String("lock/") + ID + "/cmd/open";
    String topicAdd = String("lock/") + ID + "/cmd/add";
    client.subscribe(topicOpen.c_str());
    client.subscribe(topicAdd.c_str());
  } 
  else {
    Serial.println("MQTT Connection failed");
  }
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

  client.setServer("tfm.4m1g0.com", 1883);
  client.setCallback(callback);
  delay(100);
}

void loop()
{
  int code = fingerSensor.getFingerprintID();

  if (code >= 0) { // Fingerprint found
    if (client.connected()) {
      String topic = String("lock/") + ID + "/open";
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
}

