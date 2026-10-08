#pragma once
#include <stdint.h>

class SimpleButton
{
    private:
        uint8_t pin_;
        uint16_t debounceUs_;
        bool lastValue_ = false;
        uint32_t lastChangedUs_ = 0;

    public:
        SimpleButton(uint8_t pin, uint16_t debounceUs = 50000);
        void init();
        bool pressed();
};
