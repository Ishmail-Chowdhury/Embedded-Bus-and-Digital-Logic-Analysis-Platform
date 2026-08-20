#include <Wire.h>
#include "config.h"
#include "gpio_controller.h"
#include "registers.h"
#include "interrupt_controller.h"

GPIOController gpioController;
InterruptController interruptController;
RegisterMap registerMap(gpioController, interruptController);

void receiveEvent(int byteCount);
void requestEvent();

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  gpioController.begin();
  interruptController.begin();
  registerMap.begin();

  Wire.begin(PERIPHERAL_ADDRESS);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

  Serial.println("External GPIO Peripheral ready");
  Serial.println("Register map active");
}

void loop() {
  static unsigned long lastPoll = 0;
  unsigned long now = millis();

  if (now - lastPoll > 50) {
    lastPoll = now;
    registerMap.updateStatusFromInputs();
  }

  if (interruptController.hasPendingInterrupt()) {
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void receiveEvent(int byteCount) {
  if (byteCount < 2) {
    return;
  }

  uint8_t address = Wire.read();
  uint8_t value = Wire.read();
  registerMap.writeRegister(address, value);

  Serial.print("WRITE register=0x");
  Serial.print(address, HEX);
  Serial.print(" value=0x");
  Serial.println(value, HEX);
}

void requestEvent() {
  if (Wire.available() == 0) {
    uint8_t address = 0x00;
    Wire.write(registerMap.readRegister(address));
    return;
  }

  uint8_t address = Wire.read();
  Wire.write(registerMap.readRegister(address));

  Serial.print("READ register=0x");
  Serial.println(address, HEX);
}
