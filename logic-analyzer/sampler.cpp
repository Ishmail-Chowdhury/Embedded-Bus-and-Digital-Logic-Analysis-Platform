#include "sampler.h"
#include "config.h"

void initSampler()
{
    pinMode(CH0_PIN, INPUT);
    pinMode(CH1_PIN, INPUT);
    pinMode(CH2_PIN, INPUT);
    pinMode(CH3_PIN, INPUT);
    pinMode(CH4_PIN, INPUT);
    pinMode(CH5_PIN, INPUT);
    pinMode(CH6_PIN, INPUT);
    pinMode(CH7_PIN, INPUT);
}

uint8_t sampleChannels()
{
#if defined(__AVR_ATmega328P__)
    const uint8_t portD = PIND;
    const uint8_t portC = PINC;

    uint8_t state = 0;
    state |= ((portD >> 2) & 0x01) << 0;
    state |= ((portD >> 3) & 0x01) << 1;
    state |= ((portD >> 4) & 0x01) << 2;
    state |= ((portD >> 5) & 0x01) << 3;
    state |= ((portD >> 6) & 0x01) << 4;
    state |= ((portD >> 7) & 0x01) << 5;
    state |= ((portC >> 0) & 0x01) << 6;
    state |= ((portC >> 1) & 0x01) << 7;
    return state;
#else
    uint8_t state = 0;
    state |= (digitalRead(CH0_PIN) == HIGH) ? (1 << 0) : 0;
    state |= (digitalRead(CH1_PIN) == HIGH) ? (1 << 1) : 0;
    state |= (digitalRead(CH2_PIN) == HIGH) ? (1 << 2) : 0;
    state |= (digitalRead(CH3_PIN) == HIGH) ? (1 << 3) : 0;
    state |= (digitalRead(CH4_PIN) == HIGH) ? (1 << 4) : 0;
    state |= (digitalRead(CH5_PIN) == HIGH) ? (1 << 5) : 0;
    state |= (digitalRead(CH6_PIN) == HIGH) ? (1 << 6) : 0;
    state |= (digitalRead(CH7_PIN) == HIGH) ? (1 << 7) : 0;
    return state;
#endif
}
