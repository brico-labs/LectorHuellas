#include "FingerprintSensor.h"
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include "StatusLeds/StatusLeds.h"

FingerprintSensor::FingerprintSensor()
: serial(RX, TX)
, finger(&serial)
{ 
  
}

void FingerprintSensor::begin()
{
  finger.begin(57600);
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    delay(5000);
    ESP.restart();
  }
}

int8_t FingerprintSensor::getFingerprintID() {
  int8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      //Serial.println("No finger detected");
      return -1;
    case FINGERPRINT_PACKETRECIEVEERR:
      //Serial.println("Communication error");
      return -1;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return -1;
    default:
      Serial.println("Unknown error");
      return -1;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      //Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return -1;
    case FINGERPRINT_PACKETRECIEVEERR:
      //Serial.println("Communication error");
      return -1;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return -1;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return -1;
    default:
      Serial.println("Unknown error");
      return -1;
  }
  
  // OK converted!
  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    //Serial.println("Communication error");
    return -1;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    return -2;
  } else {
    Serial.println("Unknown error");
    return -1;
  }   
  
  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID); 
  Serial.print(" with confidence of "); Serial.println(finger.confidence); 

  return finger.fingerID;
}

int8_t FingerprintSensor::wait4Finger(){
  int p = -1;
  int seconds = 0;
  long init_time = millis();
  bool ledStatus = 0;

  while (true) {
    long current_time = millis();
    p = finger.getImage();

    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("\nImage taken");
        StatusLeds::off(GREEN_LED);
        StatusLeds::off(RED_LED);
        return 0;
      case FINGERPRINT_NOFINGER:
        if ((current_time - init_time) > 500) {
          if (ledStatus){
            StatusLeds::on(GREEN_LED);
            StatusLeds::on(RED_LED);
            seconds++;
          }
          else {
            StatusLeds::off(GREEN_LED);
            StatusLeds::off(RED_LED);
          }
          
          ledStatus = !ledStatus;
          init_time = current_time;
        }

        if (seconds > 10) {
          Serial.println("\nTimeout.");
          StatusLeds::off(GREEN_LED);
          StatusLeds::off(RED_LED);
          return -1;
        }
        continue;
      case FINGERPRINT_PACKETRECIEVEERR:
        StatusLeds::off(GREEN_LED);
        StatusLeds::off(RED_LED);
        //Serial.println("\nCommunication error");
        continue;
      case FINGERPRINT_IMAGEFAIL:
        StatusLeds::off(GREEN_LED);
        StatusLeds::off(RED_LED);
        Serial.println("\nImaging error");
        return -3;
      default:
        StatusLeds::off(GREEN_LED);
        StatusLeds::off(RED_LED);
        Serial.println("\nUnknown error");
        continue; 
    }
  }
}

int8_t FingerprintSensor::trainModel(uint8_t n){
  int p = finger.image2Tz(n);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return -1;
    case FINGERPRINT_PACKETRECIEVEERR:
      //Serial.println("Communication error");
      return -2;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return -3;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return -4;
    default:
      Serial.println("Unknown error");
      return -5;
  }

  return 0;
}

int8_t FingerprintSensor::fingerprintEnroll() {
  int id = getTemplateCount();
  if (id < 0)
    return id;

  int i = 0;
  int code;
  do {
    code = finger.loadModel(id);

    if (i > 10 || (code != 0 && code != 12)) // suspicious number of used positions or actual error
      return -1;
    else if (code != 12)
      id++, i++;
  } while (code == 0);


  return id;//fingerprintEnroll(id);
}

int8_t FingerprintSensor::fingerprintEnroll(uint16_t id) {
  if (id < 0) // Error obtaining the number of templates
    return id;

  Serial.println(id);
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);

  digitalWrite(BUZZER, HIGH);
  delay(100);
  digitalWrite(BUZZER, LOW);
  delay(100);
  digitalWrite(BUZZER, HIGH);
  delay(100);
  digitalWrite(BUZZER, LOW);

  if (wait4Finger() < 0) { 
    return -1;
  }
    

  if (trainModel(1) < 0) {
    return -1;
  }
  
  Serial.println("Remove finger");
  StatusLeds::on(GREEN_LED);
  digitalWrite(BUZZER, HIGH);
  delay(200);
  digitalWrite(BUZZER, LOW);

  delay(800);

  int p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  Serial.println("Place same finger again");

  if (wait4Finger() < 0) 
    return -1;

  if (trainModel(2) < 0) 
    return -1;
  
  // OK!
  Serial.print("Creating model for #");  Serial.println(id);
  
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return -p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return -p;
  } else {
    Serial.println("Unknown error");
    return -p;
  }   
  
  Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");

    StatusLeds::on(GREEN_LED);
    digitalWrite(BUZZER, HIGH);
    delay(1000);
    digitalWrite(BUZZER, LOW);
    StatusLeds::off(GREEN_LED);
    StatusLeds::off(RED_LED);
    delay(100);
    digitalWrite(BUZZER, HIGH);
    delay(200);
    digitalWrite(BUZZER, LOW);

    Serial.println(id);
    return id;
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return -p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return -p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return -p;
  } else {
    Serial.println("Unknown error");
    return -p;
  }   
}

uint8_t FingerprintSensor::deleteFingerprint(uint8_t id) {
  uint8_t p = -1;
  
  p = finger.deleteModel(id);

  if (p == FINGERPRINT_OK) {
    Serial.println("Deleted!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not delete in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.print("Unknown error: 0x"); Serial.println(p, HEX);
    return p;
  }   

  return -1;
}

int8_t FingerprintSensor::deleteFinger(uint16_t id){
  return finger.deleteModel(id);
}

int16_t FingerprintSensor::getTemplateCount() {
  long init_time = millis();
  while (finger.getTemplateCount() < 0){
    delay(100);
    if (millis() - init_time > 2000)
      return -8;
  }
  if (finger.templateCount == 0) // sometimes it returns 0 on error
    return -1;

  return finger.templateCount;
}