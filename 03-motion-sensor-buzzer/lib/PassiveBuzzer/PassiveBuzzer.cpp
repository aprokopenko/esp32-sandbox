#include "PassiveBuzzer.h"
#include <Arduino.h>

PassiveBuzzer::PassiveBuzzer(uint8_t pin)
    : _pin(pin) {}

void PassiveBuzzer::init()
{
    // tone() uses LEDC channel 0; set it up early to avoid
    // "LEDC is not initialized" error on the first tone() call
    ledcSetup(0, 1000, 10);

    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, HIGH);
}

void PassiveBuzzer::tick()
{
    if (_isPlaying && millis() - _startedMs >= _durationMs) {
        stop();
    }
}

void PassiveBuzzer::playNote(uint16_t freq, uint32_t durationMs)
{
    tone(_pin, freq);
    _isPlaying = true;
    _startedMs = millis();
    _durationMs = durationMs;
}

void PassiveBuzzer::stop()
{
    noTone(_pin);
    // MH-FMD is active LOW: keep the pin HIGH so no current flows while idle
    digitalWrite(_pin, HIGH);
    _isPlaying = false;
}

bool PassiveBuzzer::isPlaying() const
{
    return _isPlaying;
}
