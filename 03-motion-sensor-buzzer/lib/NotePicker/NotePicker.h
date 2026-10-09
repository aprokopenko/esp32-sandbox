#pragma once
#include <stddef.h>
#include <stdint.h>

struct Note
{
    const char* name;
    uint16_t freq;
};

class NotePicker
{
private:
    const Note* _notes;
    size_t _count;

public:
    NotePicker();
    NotePicker(const Note* notes, size_t count);

    const Note& pickRandom() const;
    const Note* findByName(const char* name) const;
    size_t count() const;
};
