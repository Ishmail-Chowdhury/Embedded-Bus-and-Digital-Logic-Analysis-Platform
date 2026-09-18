#include <Wire.h>

// 10 kHz lets the Uno software sniffer observe the host link.
// Set to 100000 for normal operation without the software sniffer.
#ifndef HOST_I2C_CLOCK_HZ
#define HOST_I2C_CLOCK_HZ 10000UL
#endif
#if !defined(__AVR_ATmega328P__) || F_CPU != 16000000UL
#error "Host clock configuration requires a 16 MHz ATmega328P"
#endif

static const uint8_t PERIPHERAL_ADDRESS = 0x20;
static const uint8_t REG_GPIO0_OUTPUT = 0x00;
static const uint8_t REG_GPIO0_INPUT = 0x02;
static const uint8_t REG_DIRECTION0 = 0x04;
static const uint8_t REG_STATUS = 0x06;
static const uint8_t REG_INTERRUPT_STATUS = 0x08;
static const uint8_t REG_INTERRUPT_ENABLE = 0x09;
static const uint8_t REG_DEVICE_ID = 0x0A;

static void configureBusClock() {
#if HOST_I2C_CLOCK_HZ == 10000UL
    // SCL = F_CPU / (16 + 2 * TWBR * prescaler). setClock(10000)
    // alone overflows the 8-bit TWBR register on a 16 MHz Uno.
    TWSR = (TWSR & ~(_BV(TWPS0) | _BV(TWPS1))) | _BV(TWPS0);
    TWBR = 198; // Prescaler 4: 16 MHz / (16 + 2 * 198 * 4) = 10 kHz.
#elif HOST_I2C_CLOCK_HZ == 100000UL
    TWSR &= ~(_BV(TWPS0) | _BV(TWPS1));
    Wire.setClock(100000);
#else
#error "Supported host bus rates are 10000 and 100000 Hz"
#endif
}

static void recoverTimeoutClock() {
    if (Wire.getWireTimeoutFlag()) {
        // AVR Wire restores TWBR after reset, but resets the TWSR prescaler.
        Wire.clearWireTimeoutFlag();
        configureBusClock();
    }
}

bool writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(PERIPHERAL_ADDRESS);
    Wire.write(reg); Wire.write(value);
    const uint8_t status = Wire.endTransmission();
    const bool ok = status == 0 && !Wire.getWireTimeoutFlag();
    recoverTimeoutClock();
    if (!ok) Serial.println(F("I2C write failed"));
    return ok;
}

bool readRegister(uint8_t reg, uint8_t& value) {
    Wire.beginTransmission(PERIPHERAL_ADDRESS);
    Wire.write(reg);
    bool ok = Wire.endTransmission(false) == 0 && !Wire.getWireTimeoutFlag();
    if (ok) ok = Wire.requestFrom(PERIPHERAL_ADDRESS, uint8_t(1)) == 1 && !Wire.getWireTimeoutFlag();
    recoverTimeoutClock();
    if (!ok) {
        Serial.println(F("I2C read failed"));
        return false;
    }
    value = static_cast<uint8_t>(Wire.read());
    return true;
}

void setup() {
    Serial.begin(115200);
    pinMode(2, INPUT_PULLUP); // Peripheral D2 interrupt output -> host D2.
    Wire.begin();
    configureBusClock();
    Wire.setWireTimeout(25000, true);
    delay(500);
    uint8_t deviceId;
    if (!readRegister(REG_DEVICE_ID, deviceId) || deviceId != 0x42) {
        Serial.println(F("Peripheral missing or wrong device ID; reset after checking wiring"));
        return;
    }
    Serial.println(F("Peripheral ID 0x42 OK"));
    // P0..P3 outputs, P4..P7 switch inputs. Preload latch before enabling outputs.
    writeRegister(REG_GPIO0_OUTPUT, 0x05);
    writeRegister(REG_DIRECTION0, 0xF0);
    writeRegister(REG_INTERRUPT_STATUS, 0x03);
    writeRegister(REG_INTERRUPT_ENABLE, 0x03);
}

void loop() {
    uint8_t status, input, pending;
    if (readRegister(REG_STATUS, status)) {
        if (status & 0x04) Serial.println(F("Expander bus error: cached inputs may be stale"));
        if (readRegister(REG_GPIO0_INPUT, input)) {
            Serial.print(F("GPIO0=0x")); Serial.println(input, HEX);
        }
        if ((status & 0x02) && readRegister(REG_INTERRUPT_STATUS, pending)) {
            Serial.print(F("Changed banks=0x")); Serial.println(pending, HEX);
            writeRegister(REG_INTERRUPT_STATUS, pending);
        }
    }
    delay(100);
}
