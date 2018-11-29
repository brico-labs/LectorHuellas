#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>

class FingerprintSensor {
public:
    FingerprintSensor();


private:
    const uint8_t RX = D3;
    const uint8_t T X = D4;
    Adafruit_Fingerprint finger(RX, TX);
    SoftwareSerial serial;

};