#include "fast_capture.h"
#include <Arduino.h>
#include <avr/interrupt.h>
#if !defined(__AVR_ATmega328P__) || F_CPU != 16000000UL
#error "Fast capture requires a 16 MHz ATmega328P"
#endif

__attribute__((noinline)) uint16_t captureFastBus(uint8_t* bytes, uint16_t capacity) {
    if (!bytes || !capacity || capacity > FAST_CAPTURE_BYTES) return 0;
    const uint8_t savedSreg = SREG;
    cli();
    uint8_t previous = PINC & 0x30;
    if (previous != 0x30) { SREG = savedSreg; return 0; }
    // Timer1 belongs exclusively to this mode. Overflow provides a one-second
    // deadline without spending timer-arithmetic cycles in the START detector.
    TCCR1A = 0; TCCR1B = 0; TIMSK1 = 0;
    TCNT1 = 65536UL - 62500UL;
    TIFR1 = _BV(TOV1); TCCR1B = _BV(CS12); // 16 us ticks.
    while (true) {
        const uint8_t current = PINC & 0x30;
        if (previous == 0x30 && current == 0x20) break;
        previous = current;
        if (TIFR1 & _BV(TOV1)) {
            TCCR1B = 0; SREG = savedSreg; return 0;
        }
    }
    uint8_t first, second;
    uint8_t* destination = bytes;
    uint16_t remaining = capacity;
    // Each IN is exactly 16 CPU cycles after the previous IN, including the
    // taken loop branch. Verified against linked AVR instructions by the build.
    asm volatile(
        ".global fast_i2c_sample_loop\n"
        "fast_i2c_sample_loop:\n"
        "in %[first], %[port]\n"
        "andi %[first], 0x30\n"
        ".rept 14\n nop\n .endr\n"
        "in %[second], %[port]\n"
        "swap %[second]\n"
        "andi %[second], 0x03\n"
        "or %[first], %[second]\n"
        "st X+, %[first]\n"
        "sbiw %[remaining], 1\n"
        ".rept 6\n nop\n .endr\n"
        "brne fast_i2c_sample_loop\n"
        ".global fast_i2c_sample_end\n"
        "fast_i2c_sample_end:\n"
        : [first] "=&d" (first), [second] "=&d" (second),
          [destination] "+x" (destination), [remaining] "+w" (remaining)
        : [port] "I" (_SFR_IO_ADDR(PINC))
        : "cc", "memory"
    );
    TCCR1B = 0; SREG = savedSreg;
    return capacity * 2;
}
