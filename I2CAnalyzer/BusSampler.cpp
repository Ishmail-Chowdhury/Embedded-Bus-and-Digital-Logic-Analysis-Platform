#include "bus_sampler.h"
#include "config.h"
#include <Arduino.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#if !defined(__AVR_ATmega328P__) || F_CPU != 16000000UL
#error "I2C capture requires a 16 MHz ATmega328P (Uno R3)"
#endif
static_assert((EDGE_QUEUE_SIZE & (EDGE_QUEUE_SIZE - 1)) == 0 && EDGE_QUEUE_SIZE <= 256,
              "Edge queue size must be a power of two <= 256");
namespace {
volatile uint8_t states[EDGE_QUEUE_SIZE];
volatile uint32_t times[EDGE_QUEUE_SIZE];
uint32_t originUs = 0;
volatile uint8_t head = 0, tail = 0;
volatile bool overflowed = false;
}
ISR(PCINT1_vect) {
    const uint8_t pins = PINC; // SDA and SCL from the same port snapshot.
    const uint8_t next = (head + 1) & (EDGE_QUEUE_SIZE - 1);
    if (next == tail) { overflowed = true; PCMSK1 = 0; return; }
    times[head] = micros() - originUs;
    states[head] = pins;
    head = next;
}
void initBusSampler() { pinMode(A4, INPUT); pinMode(A5, INPUT); }
BusState readBus() {
    const uint8_t pins = PINC;
    return {(pins & _BV(PC4)) != 0, (pins & _BV(PC5)) != 0};
}
void startBusCapture() {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        head = tail = 0; overflowed = false; originUs = micros();
        PCIFR = _BV(PCIF1);
        PCMSK1 = _BV(PCINT12) | _BV(PCINT13);
        PCICR |= _BV(PCIE1);
    }
}
void stopBusCapture() {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { PCMSK1 = 0; PCICR &= ~_BV(PCIE1); }
}
bool nextBusState(BusState& state) {
    if (head == tail) return false;
    const uint8_t pins = states[tail];
    const uint32_t atUs = times[tail];
    tail = (tail + 1) & (EDGE_QUEUE_SIZE - 1);
    state = {(pins & _BV(PC4)) != 0, (pins & _BV(PC5)) != 0, atUs};
    return true;
}
bool busCaptureOverflowed() { return overflowed; }

bool busCaptureIdleTime(uint32_t& atUs) {
    bool idle;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        atUs = micros() - originUs;
        idle = head == tail;
    }
    return idle;
}
