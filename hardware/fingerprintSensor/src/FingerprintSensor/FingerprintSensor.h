#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>

class FingerprintSensor {
public:
    FingerprintSensor();
    void begin();
    int8_t getFingerprintID();

private:
    const uint8_t RX = D3;
    const uint8_t TX = D4;
    Adafruit_Fingerprint finger;
    SoftwareSerial serial;
    int8_t wait4Finger();
    int8_t trainModel(uint8_t n);
    uint8_t fingerprintEnroll(uint8_t id);
    uint8_t deleteFingerprint(uint8_t id);
    
};