#pragma once
#include <stdint.h>

class SimpleButton
{
    private:
        const uint8_t _pin;
        const uint16_t _debounceUs;
        bool _lastValue = false;
        uint32_t _lastChangedUs = 0;

    public:
        explicit SimpleButton(uint8_t pin, uint16_t debounceUs = 50000);
        void init();
        bool pressed();
};
