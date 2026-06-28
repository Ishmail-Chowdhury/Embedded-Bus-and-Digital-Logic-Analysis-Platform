#include "bus_sampler.h"
#include <Arduino.h>

static const int SDA_PIN = A4;
static const int SCL_PIN = A5;

void initBusSampler()
{
    pinMode(SDA_PIN, INPUT_PULLUP);
    pinMode(SCL_PIN, INPUT_PULLUP);
}

BusState readBus()
{
    BusState state;
    state.sda = digitalRead(SDA_PIN) != LOW;
    state.scl = digitalRead(SCL_PIN) != LOW;
    return state;
}
