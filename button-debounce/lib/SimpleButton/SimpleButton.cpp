#include <Arduino.h>
#include "SimpleButton.h"

SimpleButton::SimpleButton(uint8_t pin, uint16_t debounceUs) 
    : pin_(pin), debounceUs_(debounceUs) {}

void SimpleButton::init()
{
    pinMode(pin_, INPUT_PULLUP);
}

bool SimpleButton::pressed()
{
    bool value = digitalRead(pin_) == LOW;
    uint32_t now = micros();

    if (lastValue_ != value && (now - lastChangedUs_) > debounceUs_) {
        lastChangedUs_ = now;
        lastValue_ = value;

        if (value) {
            return true;
        }
    }

    return false;
}
