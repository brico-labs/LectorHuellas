#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>

class FingerprintSensor {
public:
  FingerprintSensor();
  void begin();
  int8_t getFingerprintID();
  int8_t fingerprintEnroll();
  int8_t fingerprintEnroll(uint16_t id);
  int8_t deleteFinger(uint16_t id);
  int16_t getTemplateCount();


private:
  const uint8_t RX = D3;
  const uint8_t TX = D4;
  SoftwareSerial serial;
  Adafruit_Fingerprint finger;
  int8_t wait4Finger();
  int8_t trainModel(uint8_t n);
  uint8_t deleteFingerprint(uint8_t id);
};