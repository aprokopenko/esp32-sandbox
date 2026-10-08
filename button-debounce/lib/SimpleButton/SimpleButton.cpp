#include <Arduino.h>
#include "SimpleButton.h"

SimpleButton::SimpleButton(uint8_t pin, uint16_t debounceUs) 
    : _pin(pin), _debounceUs(debounceUs) {}

void SimpleButton::init()
{
    pinMode(_pin, INPUT_PULLUP);
}

bool SimpleButton::pressed()
{
    bool value = digitalRead(_pin) == LOW;
    uint32_t now = micros();

    if (_lastValue != value && (now - _lastChangedUs) > _debounceUs) {
        _lastChangedUs = now;
        _lastValue = value;

        if (value) {
            return true;
        }
    }

    return false;
}
