#include "expander_bus.h"
#include "config.h"

namespace {
void low(uint8_t pin) { digitalWrite(pin, LOW); pinMode(pin, OUTPUT); }
void release(uint8_t pin) { pinMode(pin, INPUT); digitalWrite(pin, LOW); }
void halfCycle() { delayMicroseconds(5); } // SCL <= 100 kHz, plus GPIO overhead
}

void ExpanderBus::begin() {
    release(EXPANDER_SDA_PIN);
    release(EXPANDER_SCL_PIN);
    // Recover a target left in a byte after the Uno resets.
    for (uint8_t i = 0; i < 9 && digitalRead(EXPANDER_SDA_PIN) == LOW; ++i) {
        low(EXPANDER_SCL_PIN); halfCycle();
        if (!clockHigh()) break;
        halfCycle();
    }
    stop();
}

bool ExpanderBus::clockHigh() {
    release(EXPANDER_SCL_PIN);
    const uint32_t started = micros();
    while (digitalRead(EXPANDER_SCL_PIN) == LOW) {
        if (static_cast<uint32_t>(micros() - started) >= 1000) return false;
    }
    return true;
}

bool ExpanderBus::start() {
    release(EXPANDER_SDA_PIN);
    if (!clockHigh()) return false;
    halfCycle();
    if (digitalRead(EXPANDER_SDA_PIN) == LOW) return false;
    low(EXPANDER_SDA_PIN); halfCycle();
    low(EXPANDER_SCL_PIN);
    return true;
}

bool ExpanderBus::stop() {
    low(EXPANDER_SCL_PIN);
    low(EXPANDER_SDA_PIN); halfCycle();
    const bool ok = clockHigh();
    halfCycle(); release(EXPANDER_SDA_PIN); halfCycle();
    return ok && digitalRead(EXPANDER_SDA_PIN) == HIGH;
}

bool ExpanderBus::writeByte(uint8_t value) {
    for (uint8_t mask = 0x80; mask; mask >>= 1) {
        if (value & mask) release(EXPANDER_SDA_PIN);
        else low(EXPANDER_SDA_PIN);
        halfCycle();
        if (!clockHigh()) return false;
        halfCycle(); low(EXPANDER_SCL_PIN);
    }
    release(EXPANDER_SDA_PIN); halfCycle();
    if (!clockHigh()) return false;
    halfCycle();
    const bool ack = digitalRead(EXPANDER_SDA_PIN) == LOW;
    low(EXPANDER_SCL_PIN);
    return ack;
}

bool ExpanderBus::writePort(uint8_t address, uint8_t value) {
    const bool ok = start() && writeByte(address << 1) && writeByte(value);
    const bool stopped = stop();
    return ok && stopped;
}

bool ExpanderBus::readPort(uint8_t address, uint8_t& value) {
    if (!start() || !writeByte((address << 1) | 1)) { stop(); return false; }
    uint8_t received = 0;
    release(EXPANDER_SDA_PIN);
    for (uint8_t i = 0; i < 8; ++i) {
        halfCycle();
        if (!clockHigh()) { stop(); return false; }
        halfCycle();
        received = (received << 1) | (digitalRead(EXPANDER_SDA_PIN) == HIGH);
        low(EXPANDER_SCL_PIN);
    }
    // NACK the single byte: release SDA during the ninth clock.
    halfCycle();
    const bool ok = clockHigh();
    halfCycle(); low(EXPANDER_SCL_PIN);
    const bool stopped = stop();
    if (ok && stopped) value = received;
    return ok && stopped;
}
