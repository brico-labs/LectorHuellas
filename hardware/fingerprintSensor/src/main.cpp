#include "FingerprintSensor/FingerprintSensor.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "config/config.h"
#include "StatusLeds/StatusLeds.h"
#include <ESP8266httpUpdate.h>
#include <ArduinoJson.h>
#include "FS.h"
#include "Clock/Clock.h"

const short DOOR = D8;
const short CLOSE_SENSOR = D1;
const short PIR_SENSOR = D2;
const char* QUEUE_FILENAME = "msg_queue.json";
unsigned long lastTimeUpdate = 0;
unsigned long lastUpdateTime = 0;
unsigned long lastCheckQueue = 0;
unsigned long lastOpenTeleTime = 0;
unsigned long lastMovementTeleTime = 0;
bool doorOpen = false;
bool movementDetected = false;

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
    String topicStatus = String("/cmdn/lock/") + ESP.getChipId() + "/status";
    String topicDownload = String("/cmdn/lock/") + ESP.getChipId() + "/download";
    String topicUpload = String("/cmdn/lock/") + ESP.getChipId() + "/upload";
    client.subscribe(topicOpen.c_str());
    client.subscribe(topicAdd.c_str());
    client.subscribe(topicRemove.c_str());
    client.subscribe(topicStatus.c_str());
    client.subscribe(topicDownload.c_str());
    client.subscribe(topicUpload.c_str());
  } 
  else {  
    Serial.println("MQTT Connection failed");
  }
}

void printHex(int num, int precision) {
    char tmp[16];
    char format[128];
 
    sprintf(format, "%%.%dX", precision);
 
    sprintf(tmp, format, num);
    Serial.print(tmp);
}

void callback(char* topic, byte* payload, unsigned int length) {
  char payloadText[3];
  String topicOpen = String("/cmdn/lock/") + ESP.getChipId() + "/open";
  String topicAdd = String("/cmdn/lock/") + ESP.getChipId() + "/add";
  String topicRemove = String("/cmdn/lock/") + ESP.getChipId() + "/remove";
  String topicStatus = String("/cmdn/lock/") + ESP.getChipId() + "/status";
  String topicDownload = String("/cmdn/lock/") + ESP.getChipId() + "/download";
  String topicUpload = String("/cmdn/lock/") + ESP.getChipId() + "/upload";

  if (!strcmp(topic, topicOpen.c_str())) {
    Serial.println("Opening from MQTT Command");
    open();
  } 
  else if (!strcmp(topic, topicAdd.c_str())) {
    Serial.println("Adding new fingerprint to the system");

    if (length > 2) {
      Serial.println("Mensaje mqtt incorrecto");
      return;
    }

    int8_t id = 0;

    if (length == 0) {
      id = fingerSensor.fingerprintEnroll();
    } 
    else {
      memcpy(payloadText, payload, length);
      payloadText[length] = '\0';
      id = atoi(payloadText);
      if (id < 1 || id > 99)
        return;
      
      id = fingerSensor.fingerprintEnroll(id);
    }
    
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
  else if (!strcmp(topic, topicStatus.c_str())) {
    String topic = String("/stat/lock/") + String(ESP.getChipId()) + "/STATUS5";

    String reply = "{\"StatusNET\":{\"Hostname\":\"";
    reply += WiFi.hostname();
    reply += "\", \"IPAddress\":\"";
    reply += WiFi.localIP().toString();
    reply += "\", \"Gateway\":\"";
    reply += WiFi.gatewayIP().toString();
    reply += "\", \"Subnetmask\":\"";
    reply += WiFi.subnetMask().toString();
    reply += "\", \"DNSServer\":\"";
    reply += WiFi.dnsIP().toString();
    reply += "\", \"Mac\":\"";
    reply += WiFi.macAddress();
    reply += "\"}}";

    if (client.connected()) {
       Serial.println(topic);
       Serial.println(reply);
      client.publish(topic.c_str(), reply.c_str());
    } 
  }
  else if (!strcmp(topic, topicDownload.c_str())) {
    if (length > 2) {
      Serial.println("Mensaje mqtt incorrecto");
      return;
    }

    memcpy(payloadText, payload, length);
    payloadText[length] = '\0';
    uint8_t fingerTemplate[512];
    if (!fingerSensor.downloadFingerprintTemplate(atoi(payloadText), fingerTemplate)) {
      Serial.print("Huella Descargada: ");
      Serial.println(payloadText);

      for (int i = 0; i < 512; ++i) {
        printHex(fingerTemplate[i], 2);
      }
      String topic = String("/stat/lock/") + String(ESP.getChipId()) + "/enrollingHash";
      if (client.connected()) {
        Serial.println("\npublishing hash in mqtt.");
        client.publish((topic + "1").c_str(), fingerTemplate, 128);
        client.publish((topic + "2").c_str(), &fingerTemplate[128], 128);
      }
      Serial.println("\ndone.");
    } 
    else {
      Serial.print("Hubo un error al descargar la huella: ");
      Serial.println(payloadText);
    }
  }
  else if (!strcmp(topic, topicUpload.c_str())) {

  }

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}



void setup()
{
  SPIFFS.begin();
  Serial.begin(115200);
  delay(100);
  fingerSensor.begin();
  StatusLeds::begin();
  pinMode(D5, INPUT_PULLUP);
  pinMode(DOOR, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(CLOSE_SENSOR, INPUT_PULLUP);
  pinMode(PIR_SENSOR, INPUT);

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

   int i = 0;
  while (!Clock::updateTime() && i++ < 3)
  {
    Serial.println("Unable to get time. ");
    delay(500);
  }
  lastTimeUpdate = millis();

  client.setServer(MQTT_ADDR, 1883);
  client.setCallback(callback);
  delay(100);

  /*while (!client.connected()) { // Reconnect to MQTT if necesary
    static unsigned long startTime = millis();
    if (millis() - startTime > 1000 * 60 * 2) // 2 min
      ESP.restart();
    reconnect();
  }*/
}

void loop()
{
  int code = fingerSensor.getFingerprintID();

  if (code >= 0) { // Fingerprint found
    char payload[3] = "";
    itoa(code,payload, 10);

    if (client.connected()) {
      Serial.println(Clock::getUnixTime());
      String topic = String("/stat/lock/") + String(ESP.getChipId()) + "/open";
      client.publish(topic.c_str(), payload);
    }
    else {
      File file = SPIFFS.open(QUEUE_FILENAME, "a");
      if (!file)
      {
        Serial.println("Error while reading config file.");
      } else {
        StaticJsonDocument<200> root;
        root["time"] = Clock::getUnixTime();
        root["command"] = "open";
        root["id"] = payload;
        serializeJson(root, Serial);
        serializeJson(root, file);
        file.println();
        file.close();
        Serial.println("Storing message in Queue");
      }
    }

    open();
  }
  else if (code == -2) { // Fingeprint not found
    if (client.connected()) {
      String topic = String("/stat/lock/") + String(ESP.getChipId()) + "/denied";
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

  short open = digitalRead(CLOSE_SENSOR);
  if (open != doorOpen || millis() - lastOpenTeleTime > 300000 || lastOpenTeleTime == 0) {
    Serial.println("Send Tele");
    lastOpenTeleTime = millis();
    String topic;
    if (open != doorOpen)
      topic = String("/stat/lock/") + String(ESP.getChipId()) + "/status";
    else 
      topic = String("/tele/lock/") + String(ESP.getChipId()) + "/status";
    doorOpen = open;

    if (client.connected()) {
      if (doorOpen) 
        client.publish(topic.c_str(), "1");
      else
        client.publish(topic.c_str(), "0");
    }
    else {
      // TODO: we have to persist this and send log when the connection is available
    }
  }

  short movement = digitalRead(PIR_SENSOR);
  if (movement != movementDetected || millis() - lastMovementTeleTime > 300000 || lastMovementTeleTime == 0) {
    Serial.println("Send Tele");
    lastMovementTeleTime = millis();
    String topic;
    if (movement != movementDetected)
      topic = String("/stat/lock/") + String(ESP.getChipId()) + "/movement";
    else 
      topic = String("/tele/lock/") + String(ESP.getChipId()) + "/movement";
    movementDetected = movement;

    if (client.connected()) {
      if (movementDetected) 
        client.publish(topic.c_str(), "1");
      else
        client.publish(topic.c_str(), "0");
    }
    else {
      // TODO: we have to persist this and send log when the connection is available
    }
  }

  // update time via NTP
  if ((unsigned long)(millis() - lastTimeUpdate) > 1000*60*60*5)
  {
    Serial.println("Clock update...");
    Clock::updateTime();
    lastTimeUpdate = millis();
  }

  // check queue to free it
  if ((unsigned long)(millis() - lastCheckQueue) > 1000*60) {
    lastCheckQueue = millis();
    File file = SPIFFS.open(QUEUE_FILENAME, "r");

    if (file && client.connected())
    {
      Serial.print("Sending queued information.");
      while (file.available()) {
        String message = file.readStringUntil('\n');
        String topic = String("/tele/lock/") + String(ESP.getChipId()) + "/old";
        client.publish(topic.c_str(), message.c_str());
      }
    }

    file.close();
    SPIFFS.remove(QUEUE_FILENAME);
  }
  
  
}

