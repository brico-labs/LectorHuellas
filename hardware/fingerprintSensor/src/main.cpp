#include "FingerprintSensor/FingerprintSensor.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "config/password.h"


const short RED_LED = D6;
const short GREEN_LED = D7;
const short DOOR = D2;
const short BUZZER = D5;

FingerprintSensor fingerSensor;
WiFiClient wifiClient;
PubSubClient client(wifiClient);


/*void blink_both(){
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  delay(200);
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, HIGH);
  delay(200);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  delay(200);
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, HIGH);
  delay(200);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  delay(200);
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, HIGH);
  delay(200);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
}*/

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
  String targetTopic = String("lock/") + ID + "/cmd";
  if (!strcmp(topic, targetTopic.c_str())) {
    Serial.println("Opening from MQTT Command");
    open();
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
    String targetTopic = String("lock/") + ID + "/cmd";
    client.subscribe(targetTopic.c_str());
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

  pinMode(D5, INPUT_PULLUP);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
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

  if (code >= 0) {
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
  else if (code == -2) {
    digitalWrite(RED_LED, HIGH);
    for (int i = 0; i < 4; i++) {
      digitalWrite(BUZZER, HIGH);
      delay(100);
      digitalWrite(BUZZER, LOW);
      delay(100);
    }
    delay(200);
    digitalWrite(RED_LED, LOW);
  }

  if (!client.connected()) {
    reconnect();
  }

  client.loop();

  /*if (!code) {
    Serial.print("Fingerprint id: ");
    Serial.println(finger.fingerID);
    if (digitalRead(D5) == LOW && finger.fingerID < 2) {
        finger.getTemplateCount();
        fingerprintEnroll(finger.templateCount);
    }
    else {
      digitalWrite(DOOR, HIGH);
      digitalWrite(GREEN_LED, HIGH);
      delay(1000);
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(DOOR, LOW);
    }
  } */
}

