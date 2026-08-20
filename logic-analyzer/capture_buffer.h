#pragma once

#include <Arduino.h>
#include "config.h"

struct Sample
{
    uint16_t tick;
    uint8_t state;
};

class CaptureBuffer
{
public:
    CaptureBuffer();

    void clear();
    void push(const Sample& sample);

    bool full() const;
    uint16_t size() const;
    uint16_t capacity() const;

    bool get(uint16_t index, Sample& out) const;
    int16_t findFirstTickAtOrAfter(uint16_t tick) const;

private:
    Sample buffer[CAPTURE_BUFFER_SIZE];
    uint16_t head;
    uint16_t count;
};
