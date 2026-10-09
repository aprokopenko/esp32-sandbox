#include "MotionSensor.h"
#include <Arduino.h>

MotionSensor::MotionSensor(uint8_t pin, uint16_t debounceMs, uint32_t warmupMs)
    : _pin(pin), _debounceMs(debounceMs), _warmupMs(warmupMs) {}

void MotionSensor::init()
{
    pinMode(_pin, INPUT_PULLDOWN);
    _initMs = millis();
}

bool MotionSensor::isReady() const
{
    return millis() - _initMs >= _warmupMs;
}

bool MotionSensor::isJustReady()
{
    if (_isReadyReported || !isReady()) {
        return false;
    }

    _isReadyReported = true;
    return true;
}

bool MotionSensor::isJustMoved()
{
    bool value = digitalRead(_pin) == HIGH;
    uint32_t now = millis();

    if (_lastValue != value && (now - _lastChangedMs) > _debounceMs) {
        _lastChangedMs = now;
        _lastValue = value;

        // edges are still tracked during warm-up, so a HIGH left from that time
        // is not reported as motion right after it
        if (value && isReady()) {
            return true;
        }
    }

    return false;
}

bool MotionSensor::isMoving()
{
    return digitalRead(_pin) == HIGH;
}
