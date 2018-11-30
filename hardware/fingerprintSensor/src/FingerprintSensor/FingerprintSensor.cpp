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
  Serial.println(RX);
  Serial.println(TX);
  finger.begin(115200);
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
      Serial.println("Communication error");
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
      Serial.println("Communication error");
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
    Serial.println("Communication error");
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

  while (p != FINGERPRINT_OK) {
    long current_time = millis();
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("\nImage taken");
        break;
      case FINGERPRINT_NOFINGER:
        if ((current_time - init_time) > 1000) {
          init_time = current_time;
          Serial.print(".");
          seconds++;
          if (seconds > 20) {
            Serial.println("\nTimeout.");
            return -1;
          }
        }
        continue;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("\nCommunication error");
        return -2;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("\nImaging error");
        return -3;
      default:
        Serial.println("\nUnknown error");
        return -4; 
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
      Serial.println("Communication error");
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
}

uint8_t FingerprintSensor::fingerprintEnroll() {
  finger.getTemplateCount();
  uint8_t id = finger.templateCount;
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  
  StatusLeds::blinkBoth();

  if (wait4Finger() < 0) {
    return -1;
  }
    

  if (trainModel(1) < 0) {
    return -1;
  }
  
  Serial.println("Remove finger");
  StatusLeds::on(GREEN_LED);
  delay(1000);
  StatusLeds::blinkBoth();

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
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
  
  Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
    StatusLeds::on(GREEN_LED);
    delay(1000);
    StatusLeds::off(GREEN_LED);
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
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
}