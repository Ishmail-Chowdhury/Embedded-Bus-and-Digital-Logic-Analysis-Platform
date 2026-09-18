#include "analog_capture.h"
#include <avr/interrupt.h>
#if !defined(__AVR_ATmega328P__) || F_CPU != 16000000UL
#error "Analog capture timing requires a 16 MHz ATmega328P"
#endif
uint16_t captureAnalog(uint8_t* bytes, uint16_t capacity) {
    if (!bytes || capacity < 2 || capacity > 512) return 0;
    const uint8_t savedSREG = SREG;
    cli();
    const uint8_t mux = ADMUX, control = ADCSRA, trigger = ADCSRB, digital = DIDR0;
    ADCSRA = 0;
    ADMUX = _BV(REFS0) | 2; // A2, AVCC reference, right-adjusted 10-bit result.
    ADCSRB = 0; DIDR0 |= _BV(ADC2D);
    ADCSRA = _BV(ADEN) | _BV(ADATE) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0) | _BV(ADIF);
    ADCSRA |= _BV(ADSC); // Free running, no ADC ISR.
    uint16_t count = 0;
    bool first = true, failed = false;
    while (count < capacity / 2) {
        uint16_t wait = 5000;
        while (!(ADCSRA & _BV(ADIF)) && --wait) {}
        if (!wait) { failed = true; break; }
        const uint16_t value = ADC;
        ADCSRA |= _BV(ADIF);
        if (first) { first = false; continue; } // Discard extended first conversion after enabling.
        bytes[count * 2] = uint8_t(value);
        bytes[count * 2 + 1] = uint8_t(value >> 8);
        ++count;
        if (ADCSRA & _BV(ADIF)) { failed = true; break; }
    }
    ADCSRA = 0; ADMUX = mux; ADCSRB = trigger; DIDR0 = digital; ADCSRA = control;
    SREG = savedSREG;
    return failed ? 0 : count;
}
