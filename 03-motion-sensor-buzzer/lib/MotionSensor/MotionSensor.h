#pragma once
#include <stdint.h>

class MotionSensor {
private:
    const uint8_t _pin;
    const uint16_t _debounceMs;
    const uint32_t _warmupMs;
    uint32_t _lastChangedMs;
    uint32_t _lastValue;
    uint32_t _initMs = 0;
    bool _isReadyReported = false;

public:
    // PIR sensor gives false triggers while it settles after power-up,
    // so motion is ignored for warmupMs after init()
    explicit MotionSensor(uint8_t pin, uint16_t debounceMs = 50, uint32_t warmupMs = 20000);
    void init();
    bool isReady() const;
    bool isJustReady();
    bool isJustMoved();
    bool isMoving();
};
