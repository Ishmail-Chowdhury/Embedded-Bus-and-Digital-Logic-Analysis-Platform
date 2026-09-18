#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <deque>
#include <functional>
#include <cassert>
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define A0 14
#define A1 15
#define A4 18
#define A5 19
#define LED_BUILTIN 13
#define HEX 16
#define F(x) x
extern uint8_t pinModes[32], pinLevels[32];
extern uint32_t fakeMicros;
extern std::function<int(uint8_t)> readHook;
inline void pinMode(uint8_t pin, uint8_t mode) { pinModes[pin] = mode; }
inline void digitalWrite(uint8_t pin, uint8_t level) { pinLevels[pin] = level; }
inline int digitalRead(uint8_t pin) { return readHook ? readHook(pin) : pinLevels[pin]; }
inline uint32_t micros() { return fakeMicros++; }
inline uint32_t millis() { return fakeMicros / 1000; }
inline void delayMicroseconds(uint32_t us) { fakeMicros += us; }
inline void delay(uint32_t ms) { fakeMicros += ms * 1000; }
struct FakeSerial {
    std::deque<char> input;
    void begin(unsigned long) {}
    int available() { return input.size(); }
    int read() { if (input.empty()) return -1; char c = input.front(); input.pop_front(); return c; }
    void flush() {}
    template <class T> void print(T) {}
    template <class T> void println(T) {}
    template <class T> void print(T, int) {}
    template <class T> void println(T, int) {}
};
extern FakeSerial Serial;

#define _BV(bit) (1U << (bit))
#define TWPS0 0
#define TWPS1 1
extern uint8_t TWSR, TWBR;
