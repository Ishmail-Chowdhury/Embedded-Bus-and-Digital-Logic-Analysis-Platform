#include "registers.h"
#include "interrupt_controller.h"
#include <util/atomic.h>

RegisterMap::RegisterMap(GPIOController& gpio, InterruptController& interrupt)
    : gpioController_(gpio), interruptController_(interrupt), registers_{},
      lastInput_{}, lastDirection_{}, inputValid_{false, false} {}

void RegisterMap::begin() {
    for (uint8_t i = 0; i < REGISTER_COUNT; ++i) registers_[i] = 0;
    registers_[REG_DIRECTION0] = registers_[REG_DIRECTION1] = 0xFF;
    registers_[REG_CONTROL] = CONTROL_ENABLED;
    registers_[REG_STATUS] = STATUS_ENABLED | STATUS_ERROR; // Until first successful poll.
    registers_[REG_DEVICE_ID] = DEVICE_ID_VALUE;
    inputValid_[0] = inputValid_[1] = false;
    syncInterrupt();
}

void RegisterMap::syncInterrupt() {
    const bool pending = (registers_[REG_INTERRUPT_STATUS] & registers_[REG_INTERRUPT_ENABLE]) != 0
                         && (registers_[REG_CONTROL] & CONTROL_ENABLED);
    if (pending) registers_[REG_STATUS] |= STATUS_INTERRUPT;
    else registers_[REG_STATUS] &= static_cast<uint8_t>(~STATUS_INTERRUPT);
    interruptController_.setPending(pending);
}

void RegisterMap::writeRegister(uint8_t address, uint8_t value) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        switch (address) {
        case REG_GPIO0_OUTPUT: case REG_GPIO1_OUTPUT:
        case REG_DIRECTION0: case REG_DIRECTION1:
            registers_[address] = value;
            break;
        case REG_CONTROL:
            registers_[address] = value & CONTROL_ENABLED;
            if (value & CONTROL_ENABLED) registers_[REG_STATUS] |= STATUS_ENABLED;
            else {
                registers_[REG_STATUS] &= static_cast<uint8_t>(~STATUS_ENABLED);
                registers_[REG_INTERRUPT_STATUS] = 0;
            }
            break;
        case REG_INTERRUPT_ENABLE:
            registers_[address] = value & 3; // Bit 0 = bank 0, bit 1 = bank 1.
            break;
        case REG_INTERRUPT_STATUS:
            registers_[address] &= static_cast<uint8_t>(~value); // Write one to clear.
            break;
        default: break; // Inputs, status, device ID and reserved addresses are read-only.
        }
        syncInterrupt();
    }
}

uint8_t RegisterMap::readRegister(uint8_t address) const {
    return address < REGISTER_COUNT ? registers_[address] : 0;
}

void RegisterMap::updateStatusFromInputs() {
    uint8_t outputs[2], directions[2], control;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        outputs[0] = registers_[REG_GPIO0_OUTPUT]; outputs[1] = registers_[REG_GPIO1_OUTPUT];
        directions[0] = registers_[REG_DIRECTION0]; directions[1] = registers_[REG_DIRECTION1];
        control = registers_[REG_CONTROL];
    }
    bool error = false;
    for (uint8_t bank = 0; bank < 2; ++bank) {
        uint8_t input = 0;
        const bool applied = gpioController_.apply(bank, outputs[bank], directions[bank], control & CONTROL_ENABLED);
        const bool readOK = gpioController_.read(bank, input);
        error |= !applied || !readOK;
        if (!applied || !readOK) { inputValid_[bank] = false; continue; }
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            registers_[REG_GPIO0_INPUT + bank] = input;
            // A concurrent host configuration change invalidates this comparison.
            const bool sameConfig = control == registers_[REG_CONTROL]
                && directions[bank] == registers_[REG_DIRECTION0 + bank];
            if (sameConfig && inputValid_[bank] && (control & CONTROL_ENABLED)
                && ((input ^ lastInput_[bank]) & directions[bank] & lastDirection_[bank])) {
                registers_[REG_INTERRUPT_STATUS] |= (1 << bank);
            }
            inputValid_[bank] = sameConfig && (control & CONTROL_ENABLED);
            lastInput_[bank] = input;
            lastDirection_[bank] = directions[bank];
            syncInterrupt();
        }
    }
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if (error) registers_[REG_STATUS] |= STATUS_ERROR;
        else registers_[REG_STATUS] &= static_cast<uint8_t>(~STATUS_ERROR);
    }
}
