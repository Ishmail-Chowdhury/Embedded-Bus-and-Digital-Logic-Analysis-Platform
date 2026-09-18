#include "registers.h"
#include "interrupt_controller.h"
#include <util/atomic.h>

RegisterMap::RegisterMap(GPIOController& gpio, InterruptController& interrupt)
    : gpioController_(gpio), interruptController_(interrupt), registers_{},
      candidate_{}, changedAt_{}, sampledRevision_{}, inputValid_{false, false} {}

void RegisterMap::begin() {
    for (uint8_t i = 0; i < REGISTER_COUNT; ++i) registers_[i] = 0;
    registers_[REG_DIRECTION0] = registers_[REG_DIRECTION1] = 0xFF;
    registers_[REG_CONTROL] = CONTROL_ENABLED;
    registers_[REG_STATUS] = STATUS_ENABLED | STATUS_ERROR; // Until first successful poll.
    registers_[REG_DEVICE_ID] = DEVICE_ID_VALUE;
    registers_[REG_DEBOUNCE_MS] = 20;
    ++revision_;
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
            registers_[address] = value;
            break;
        case REG_DIRECTION0: case REG_DIRECTION1: case REG_DEBOUNCE_MS:
            if (registers_[address] != value) ++revision_;
            registers_[address] = value;
            break;
        case REG_CONTROL:
            if (registers_[address] != (value & CONTROL_ENABLED)) ++revision_;
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
        case REG_COUNTER_CLEAR0: case REG_COUNTER_CLEAR1:
            for (uint8_t bit = 0; bit < 8; ++bit) {
                if (value & (1 << bit)) {
                    const uint8_t counter = REG_EVENT_COUNTERS + 16 * (address - REG_COUNTER_CLEAR0) + 2 * bit;
                    registers_[counter] = registers_[counter + 1] = 0;
                }
            }
            break;
        default: break; // Inputs, status, device ID and reserved addresses are read-only.
        }
        syncInterrupt();
    }
}

uint8_t RegisterMap::readRegister(uint8_t address) const {
    return address < REGISTER_COUNT ? registers_[address] : 0;
}

void RegisterMap::snapshot(uint8_t address, uint8_t* out, uint8_t length) const {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        for (uint8_t i = 0; i < length; ++i) out[i] = readRegister(uint8_t(address + i));
    }
}

void RegisterMap::updateStatusFromInputs() {
    uint8_t outputs[2], directions[2], control, debounce;
    uint16_t revision;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        outputs[0] = registers_[REG_GPIO0_OUTPUT]; outputs[1] = registers_[REG_GPIO1_OUTPUT];
        directions[0] = registers_[REG_DIRECTION0]; directions[1] = registers_[REG_DIRECTION1];
        control = registers_[REG_CONTROL]; debounce = registers_[REG_DEBOUNCE_MS];
        revision = revision_;
    }
    bool error = false;
    for (uint8_t bank = 0; bank < 2; ++bank) {
        uint8_t input = 0;
        const bool applied = gpioController_.apply(bank, outputs[bank], directions[bank], control & CONTROL_ENABLED);
        const bool readOK = gpioController_.read(bank, input);
        error |= !applied || !readOK;
        if (!applied || !readOK) { inputValid_[bank] = false; continue; }
        const uint32_t now = millis();
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            registers_[REG_RAW_INPUT0 + bank] = input;
            // Reconfiguration, reconnect and disabled ports establish a fresh baseline.
            const bool validConfig = revision == revision_ && (control & CONTROL_ENABLED);
            if (!validConfig || !inputValid_[bank] || sampledRevision_[bank] != revision) {
                candidate_[bank] = input;
                registers_[REG_GPIO0_INPUT + bank] = input;
                for (uint8_t bit = 0; bit < 8; ++bit) changedAt_[bank * 8 + bit] = now;
            } else {
                uint8_t stable = registers_[REG_GPIO0_INPUT + bank];
                for (uint8_t bit = 0; bit < 8; ++bit) {
                    const uint8_t mask = 1 << bit;
                    const uint8_t pin = bank * 8 + bit;
                    if ((input ^ candidate_[bank]) & mask) {
                        candidate_[bank] ^= mask;
                        changedAt_[pin] = now;
                    }
                    if (!((stable ^ input) & mask)) continue;
                    if (!(directions[bank] & mask)) { stable ^= mask; continue; }
                    if (uint32_t(now - changedAt_[pin]) < debounce) continue;
                    stable ^= mask;
                    registers_[REG_INTERRUPT_STATUS] |= (1 << bank);
                    const uint8_t address = REG_EVENT_COUNTERS + 2 * pin;
                    uint16_t count = registers_[address] | (uint16_t(registers_[address + 1]) << 8);
                    if (count != 0xFFFF) ++count;
                    registers_[address] = uint8_t(count);
                    registers_[address + 1] = uint8_t(count >> 8);
                }
                registers_[REG_GPIO0_INPUT + bank] = stable;
            }
            inputValid_[bank] = validConfig;
            sampledRevision_[bank] = revision;
            syncInterrupt();
        }
    }
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if (error) registers_[REG_STATUS] |= STATUS_ERROR;
        else registers_[REG_STATUS] &= static_cast<uint8_t>(~STATUS_ERROR);
    }
}
