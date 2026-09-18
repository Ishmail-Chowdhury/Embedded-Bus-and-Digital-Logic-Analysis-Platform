#include <Wire.h>
#include "config.h"
#include "gpio_controller.h"
#include "registers.h"
#include "interrupt_controller.h"

GPIOController gpioController;
InterruptController interruptController;
RegisterMap registerMap(gpioController, interruptController);
static volatile uint8_t registerPointer = 0;

void receiveEvent(int byteCount) {
    if (byteCount <= 0 || !Wire.available()) return;
    registerPointer = static_cast<uint8_t>(Wire.read());
    while (Wire.available()) {
        registerMap.writeRegister(registerPointer, static_cast<uint8_t>(Wire.read()));
        ++registerPointer;
    }
}

void requestEvent() {
    // Wire does not tell onRequest how many bytes the host will clock out.
    // Offer a coherent 32-byte window, and keep the selected start unchanged.
    uint8_t bytes[MAX_BURST_BYTES];
    registerMap.snapshot(registerPointer, bytes, sizeof(bytes));
    Wire.write(bytes, sizeof(bytes));
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(EXPANDER_INT_PIN, INPUT_PULLUP);
    gpioController.begin();
    interruptController.begin();
    registerMap.begin();
    registerMap.updateStatusFromInputs();
    Wire.begin(PERIPHERAL_ADDRESS);
    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);
}

void loop() {
    static uint32_t lastPoll = 0;
    const uint32_t now = millis();
    if (now - lastPoll >= INPUT_POLL_MS || digitalRead(EXPANDER_INT_PIN) == LOW) {
        registerMap.updateStatusFromInputs();
        lastPoll = millis();
    }
    digitalWrite(LED_BUILTIN, interruptController.hasPendingInterrupt() ? HIGH : LOW);
}
