#include "gpio_controller.h"
#include "config.h"
namespace {
uint8_t addressOf(uint8_t bank) { return bank ? PCF8574A_ADDRESS_1 : PCF8574A_ADDRESS_0; }
}
GPIOController::GPIOController() : applied_{0xFF, 0xFF}, valid_{false, false} {}
void GPIOController::begin() { bus_.begin(); }
bool GPIOController::apply(uint8_t bank, uint8_t output, uint8_t direction, bool enabled) {
    if (bank > 1) return false;
    const uint8_t value = portValue(output, direction, enabled);
    if (valid_[bank] && applied_[bank] == value) return true;
    valid_[bank] = bus_.writePort(addressOf(bank), value);
    if (valid_[bank]) applied_[bank] = value;
    return valid_[bank];
}
bool GPIOController::read(uint8_t bank, uint8_t& value) {
    if (bank > 1) return false;
    const bool ok = bus_.readPort(addressOf(bank), value);
    if (!ok) valid_[bank] = false; // Reapply outputs after reconnect.
    return ok;
}
