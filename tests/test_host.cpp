#include <cassert>
#include <cstdio>
#define __AVR_ATmega328P__ 1
#define F_CPU 16000000UL
#include "Wire.h"
#include "../ExternalGPIOPeripheral/host/host.ino"
FakeWire Wire;
int main() {
    Wire.replies.push_back({0x42}); setup();
    assert((TWSR & 3) == 1 && TWBR == 198);
    assert(F_CPU / (16 + 2 * TWBR * 4UL) == 10000UL);
    uint8_t value = 0;
    Wire.replies.push_back({0xA5});
    assert(readRegister(0x02, value) && value == 0xA5 && !Wire.lastStop);
    assert(Wire.tx.size() == 1 && Wire.tx[0] == 0x02);
    Wire.nextError = 2;
    value = 0x55; assert(!readRegister(0x02, value) && value == 0x55);
    assert(!writeRegister(0x00, 0xAA));
    Wire.nextError = 0;
    assert(!readRegister(0x02, value) && value == 0x55); // Short read.
    assert(writeRegister(0x04, 0xF0));
    assert(Wire.lastStop && Wire.tx.size() == 2 && Wire.tx[0] == 4 && Wire.tx[1] == 0xF0);
    Wire.timeout = true; TWSR = 0;
    assert(!writeRegister(0, 0));
    assert(!Wire.timeout && (TWSR & 3) == 1 && TWBR == 198);
    Wire.timeout = true; TWSR = 0;
    Wire.replies.push_back({0x11}); value = 0x55;
    assert(!readRegister(2, value) && value == 0x55);
    assert(!Wire.timeout && (TWSR & 3) == 1);
    Wire.replies.clear();
    uint8_t bytes[32] = {};
    std::deque<uint8_t> reply;
    for (uint8_t i = 0; i < 32; ++i) reply.push_back(i);
    Wire.replies.push_back(reply);
    assert(readRegisters(0x10, bytes, 32) && bytes[31] == 31);
    Wire.replies.push_back({0xAA});
    assert(!readRegisters(0x10, bytes, 32) && bytes[0] == 0 && bytes[31] == 31);
    assert(!readRegisters(0, bytes, 0) && !readRegisters(0, bytes, 33));
    puts("PASS host: 10 kHz prescaler, repeated START, NACK, short reads, timeout prescaler recovery");
}
