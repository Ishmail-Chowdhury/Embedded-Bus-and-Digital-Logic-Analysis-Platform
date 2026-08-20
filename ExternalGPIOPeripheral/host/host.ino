#include <Wire.h>

#define HOST_ADDRESS 0x20
#define REG_GPIO0_OUTPUT      0x00
#define REG_GPIO1_OUTPUT      0x01
#define REG_GPIO0_INPUT       0x02
#define REG_GPIO1_INPUT       0x03
#define REG_DIRECTION0        0x04
#define REG_DIRECTION1        0x05
#define REG_STATUS            0x06
#define REG_CONTROL           0x07
#define REG_INTERRUPT_STATUS  0x08
#define REG_INTERRUPT_ENABLE  0x09
#define REG_DEVICE_ID         0x0A

void writeRegister(uint8_t slaveAddr, uint8_t reg, uint8_t value) {
  Wire.beginTransmission(slaveAddr);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

uint8_t readRegister(uint8_t slaveAddr, uint8_t reg) {
  Wire.beginTransmission(slaveAddr);
  Wire.write(reg);
  Wire.endTransmission();

  Wire.requestFrom(static_cast<int>(slaveAddr), 1);
  if (Wire.available()) {
    return Wire.read();
  }
    return 0;
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  delay(500);

  Serial.println("Host controller ready");

  uint8_t deviceId = readRegister(HOST_ADDRESS, REG_DEVICE_ID);
  Serial.print("Device ID: 0x");
  Serial.println(deviceId, HEX);

  writeRegister(HOST_ADDRESS, REG_GPIO0_OUTPUT, 0x55);
  delay(500);

  Serial.print("GPIO0 input sample: 0x");
  Serial.println(readRegister(HOST_ADDRESS, REG_GPIO0_INPUT), HEX);

  writeRegister(HOST_ADDRESS, REG_CONTROL, 0x01);
  Serial.print("Status register: 0x");
  Serial.println(readRegister(HOST_ADDRESS, REG_STATUS), HEX);
}

void loop() {
  uint8_t status = readRegister(HOST_ADDRESS, REG_STATUS);
  if (status & 0x02) {
    Serial.print("Interrupt pending. Status = 0x");
    Serial.println(status, HEX);
  }

  delay(1000);
}
