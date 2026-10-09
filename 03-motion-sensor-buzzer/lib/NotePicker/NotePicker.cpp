#include "NotePicker.h"
#include <Arduino.h>
#include <string.h>

// visible only inside this file, reachable from outside through NotePicker
static constexpr Note DEFAULT_NOTES[] = {
    {"C5", 523},
    {"D5", 587},
    {"E5", 659},
    {"F5", 698},
    {"G5", 784},
    {"A5", 880},
    {"B5", 988},
    {"C6", 1047},
};

NotePicker::NotePicker()
    : NotePicker(DEFAULT_NOTES, sizeof(DEFAULT_NOTES) / sizeof(DEFAULT_NOTES[0])) {}

NotePicker::NotePicker(const Note* notes, size_t count)
    : _notes(notes), _count(count) {}

const Note& NotePicker::pickRandom() const
{
    return _notes[random(_count)];
}

const Note* NotePicker::findByName(const char* name) const
{
    for (size_t i = 0; i < _count; i++) {
        if (strcmp(_notes[i].name, name) == 0) {
            return &_notes[i];
        }
    }
    return nullptr;
}

size_t NotePicker::count() const
{
    return _count;
}
