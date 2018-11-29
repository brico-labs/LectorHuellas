#include "FingerprintSensor.h"
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>

FingerprintSensor::FingerprintSensor()
{
    serial = SoftwareSerial(RX, TX);
    finger = Adafruit_Fingerprint(&serial);
}

