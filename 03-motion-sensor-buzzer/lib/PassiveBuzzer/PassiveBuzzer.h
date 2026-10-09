#pragma once
#include <stdint.h>

class PassiveBuzzer
{
private:
    const uint8_t _pin;
    bool _isPlaying = false;
    uint32_t _startedMs = 0;
    uint32_t _durationMs = 0;

public:
    explicit PassiveBuzzer(uint8_t pin);
    void init();
    void tick();
    void playNote(uint16_t freq, uint32_t durationMs);
    void stop();
    bool isPlaying() const;
};
