#include <cassert>
#include <cstdio>
#include <deque>
#include "expander_bus.h"
#include "config.h"
static std::deque<int> sda;
static bool stuckClock = false;
int main() {
    readHook = [](uint8_t pin) {
        assert(!(pinModes[pin] == OUTPUT && pinLevels[pin] == HIGH));
        if (pin == EXPANDER_SCL_PIN) return stuckClock ? LOW : HIGH;
        assert(pin == EXPANDER_SDA_PIN && !sda.empty());
        int v = sda.front(); sda.pop_front(); return v;
    };
    ExpanderBus bus;
    sda = {1, 0, 0, 1}; assert(bus.writePort(0x38, 0xA5) && sda.empty());
    sda = {1, 1, 1}; assert(!bus.writePort(0x38, 0) && sda.empty()); // Address NACK.
    sda = {1, 0, 1, 1}; assert(!bus.writePort(0x38, 0) && sda.empty()); // Data NACK.
    sda = {1, 0, 1,0,1,0,0,1,0,1, 1};
    uint8_t value = 0; assert(bus.readPort(0x39, value) && value == 0xA5 && sda.empty());
    sda = {1, 1, 1}; value = 0x42;
    assert(!bus.readPort(0x39, value) && value == 0x42 && sda.empty());
    stuckClock = true;
    const uint32_t before = fakeMicros;
    assert(!bus.writePort(0x38, 0) && fakeMicros - before < 3000);
    assert(pinModes[EXPANDER_SDA_PIN] == INPUT && pinModes[EXPANDER_SCL_PIN] == INPUT);
    stuckClock = false;
    sda = {0,0,0,0,0,0,0,0,0,1}; bus.begin(); assert(sda.empty()); // 9 recovery pulses and STOP.
    puts("PASS downstream bus: ACK/NACK, byte read, unchanged data on error, bounded stuck clock, bus recovery");
}
