#include "Arduino.h"
uint8_t pinModes[32] = {}, pinLevels[32] = {};
uint32_t fakeMicros = 0;
std::function<int(uint8_t)> readHook;
FakeSerial Serial;

uint8_t TWSR = 0, TWBR = 0;
