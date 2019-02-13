#include <Arduino.h>

enum Leds {
    RED_LED = D6,
    GREEN_LED = D7,
    BUZZER = D5
};

class StatusLeds {
public:
    static void begin();
    static void on(Leds led);
    static void off(Leds led);
    static void blinkBoth();
private:
};