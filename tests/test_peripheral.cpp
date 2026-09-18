#include <cassert>
#include <cstdio>
#include <vector>
#include <functional>
#include "registers.h"
#include "interrupt_controller.h"
#include "Wire.h"
#include "../ExternalGPIOPeripheral/peripheral/peripheral.ino"
FakeWire Wire;
struct PortWrite { uint8_t address, value; };
static std::vector<PortWrite> writes;
static uint8_t inputs[2] = {0xFF, 0xFF};
static bool online[2] = {true, true};
static std::function<void()> duringRead;
void ExpanderBus::begin() {}
bool ExpanderBus::writePort(uint8_t address, uint8_t value) {
    assert(address == 0x38 || address == 0x39);
    writes.push_back({address, value}); return online[address - 0x38];
}
bool ExpanderBus::readPort(uint8_t address, uint8_t& value) {
    if (duringRead) { auto hook = duringRead; duringRead = {}; hook(); }
    const int bank = address - 0x38; assert(bank == 0 || bank == 1);
    if (!online[bank]) return false;
    value = inputs[bank]; return true;
}
static void rx(std::initializer_list<uint8_t> bytes) {
    Wire.rx = bytes; receiveEvent(bytes.size()); assert(Wire.rx.empty());
}
static uint8_t tx() { Wire.tx.clear(); requestEvent(); assert(Wire.tx.size() == 1); return Wire.tx[0]; }
int main() {
    interruptController.begin(); registerMap.begin(); registerMap.updateStatusFromInputs();
    assert(writes.size() == 2 && writes[0].value == 0xFF && writes[1].value == 0xFF);
    assert(registerMap.readRegister(REG_DIRECTION0) == 0xFF);
    assert(registerMap.readRegister(REG_CONTROL) == 1 && registerMap.readRegister(REG_STATUS) == STATUS_ENABLED);
    const auto before = writes.size();
    rx({REG_DEVICE_ID}); assert(tx() == 0x42); // Register pointer-only write.
    rx({REG_GPIO0_OUTPUT, 0x55, 0xAA}); // Sequential cached writes.
    assert(writes.size() == before); // Callback did no downstream I2C.
    assert(registerMap.readRegister(REG_GPIO0_OUTPUT) == 0x55 && registerMap.readRegister(REG_GPIO1_OUTPUT) == 0xAA);
    rx({REG_DIRECTION0, 0xF0}); registerMap.updateStatusFromInputs();
    assert(writes.back().address == 0x38 && writes.back().value == 0xF5);
    rx({REG_DEVICE_ID, 0x00}); rx({REG_GPIO0_INPUT, 0}); rx({REG_STATUS, 0});
    assert(registerMap.readRegister(REG_DEVICE_ID) == 0x42 && registerMap.readRegister(REG_GPIO0_INPUT) == 0xFF);
    rx({REG_INTERRUPT_ENABLE, 3});
    inputs[0] = 0xEF; registerMap.updateStatusFromInputs();
    assert(registerMap.readRegister(REG_INTERRUPT_STATUS) == 1);
    assert(interruptController.hasPendingInterrupt() && pinModes[INT_PIN] == OUTPUT && pinLevels[INT_PIN] == LOW);
    rx({REG_INTERRUPT_STATUS, 0}); assert(registerMap.readRegister(REG_INTERRUPT_STATUS) == 1);
    rx({REG_INTERRUPT_STATUS, 1}); assert(!interruptController.hasPendingInterrupt() && pinModes[INT_PIN] == INPUT);
    inputs[0] ^= 1; registerMap.updateStatusFromInputs(); // Output bit change does not trigger.
    assert(registerMap.readRegister(REG_INTERRUPT_STATUS) == 0);
    inputs[1] = 0xFE; registerMap.updateStatusFromInputs();
    assert(registerMap.readRegister(REG_INTERRUPT_STATUS) == 2);
    rx({REG_INTERRUPT_ENABLE, 0}); assert(!interruptController.hasPendingInterrupt());
    rx({REG_INTERRUPT_ENABLE, 3}); assert(interruptController.hasPendingInterrupt());
    rx({REG_CONTROL, 0}); assert(!interruptController.hasPendingInterrupt());
    registerMap.updateStatusFromInputs(); assert(writes.back().value == 0xFF);
    assert(!(registerMap.readRegister(REG_STATUS) & STATUS_ENABLED));
    rx({REG_CONTROL, 1}); registerMap.updateStatusFromInputs();
    online[0] = false; const uint8_t lastGood = registerMap.readRegister(REG_GPIO0_INPUT);
    registerMap.updateStatusFromInputs();
    assert((registerMap.readRegister(REG_STATUS) & STATUS_ERROR) && registerMap.readRegister(REG_GPIO0_INPUT) == lastGood);
    online[0] = true; registerMap.updateStatusFromInputs();
    assert(!(registerMap.readRegister(REG_STATUS) & STATUS_ERROR));
    rx({REG_INTERRUPT_STATUS, 3}); inputs[0] ^= 0x10;
    duringRead = [] { registerMap.writeRegister(REG_CONTROL, 0); };
    registerMap.updateStatusFromInputs(); assert(registerMap.readRegister(REG_INTERRUPT_STATUS) == 0);
    rx({0xFF, 0xAA}); // Invalid register is safely ignored; pointer wraps as an 8-bit counter.
    rx({0xFE}); assert(tx() == 0); assert(tx() == 0);
    puts("PASS peripheral: callbacks, pointers, direction, read-only registers, IRQ/W1C, disable, NACK/recovery, concurrent writes");
}
