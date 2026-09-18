#include "sampler.h"
#include "config.h"
#include <avr/interrupt.h>
#if !defined(__AVR_ATmega328P__) || F_CPU != 16000000UL
#error "100 kS/s capture requires a 16 MHz ATmega328P (Uno R3)"
#endif
static_assert(SAMPLE_INTERVAL_US == 10, "Revalidate timing before changing the capture rate");
void initSampler() {
    pinMode(CH0_PIN, INPUT); pinMode(CH1_PIN, INPUT);
    pinMode(CH2_PIN, INPUT); pinMode(CH3_PIN, INPUT);
    pinMode(CH4_PIN, INPUT); pinMode(CH5_PIN, INPUT);
    pinMode(CH6_PIN, INPUT); pinMode(CH7_PIN, INPUT);
}
static inline __attribute__((always_inline)) uint8_t readChannelsFast() {
    const uint8_t portD = PIND;
    const uint8_t portC = PINC;
    return (portD >> 2) | ((portC & 3) << 6);
}
uint8_t sampleChannels() { return readChannelsFast(); }
CaptureResult captureBurst(CaptureSession& session, const TriggerConfig& config) {
    session.begin(sampleChannels(), config);
    // Exclusive Timer1 ownership; OLED/Serial run only outside this bounded burst.
    const uint8_t savedSREG = SREG;
    cli();
    TCCR1A = 0; TCCR1B = 0; TIMSK1 = 0;
    TCNT1 = 0; OCR1A = (F_CPU / 8 / 1000000UL) * SAMPLE_INTERVAL_US - 1;
    TIFR1 = _BV(OCF1A) | _BV(TOV1);
    TCCR1B = _BV(WGM12) | _BV(CS11); // CTC at 10 us; 0.5 us timer resolution.
    CaptureResult result = CAPTURE_TIMEOUT;
    uint32_t remaining = ARM_TIMEOUT_SAMPLES;
    while (remaining--) {
        while (!(TIFR1 & _BV(OCF1A))) {}
        TIFR1 = _BV(OCF1A);
        const uint16_t phase = TCNT1;
        const uint8_t state = readChannelsFast();
        if (phase > 2) { result = CAPTURE_OVERRUN; break; }
        const bool done = session.add(state);
        // Never label a late sample as evenly timed data.
        if (TIFR1 & _BV(OCF1A)) { result = CAPTURE_OVERRUN; break; }
        if (done) { result = CAPTURE_OK; break; }
    }
    TCCR1B = 0; TIFR1 = _BV(OCF1A) | _BV(TOV1);
    SREG = savedSREG;
    if (result != CAPTURE_OK) session.buffer.clear();
    return result;
}
