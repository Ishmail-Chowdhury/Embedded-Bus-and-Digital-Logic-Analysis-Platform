#pragma once
#include "Arduino.h"
#include <vector>
struct FakeWire {
    std::deque<uint8_t> rx;
    std::vector<uint8_t> tx;
    bool timeout = false;
    bool getWireTimeoutFlag() { return timeout; }
    void clearWireTimeoutFlag() { timeout = false; }
    uint8_t nextError = 0;
    bool lastStop = true;
    std::deque<std::deque<uint8_t>> replies;
    void begin(uint8_t = 0) {}
    void setClock(uint32_t) {}
    void setWireTimeout(uint32_t, bool) {}
    void beginTransmission(uint8_t) { tx.clear(); }
    uint8_t endTransmission(bool stop = true) { lastStop = stop; return nextError; }
    uint8_t requestFrom(uint8_t, uint8_t) {
        if (replies.empty()) return 0;
        rx = replies.front(); replies.pop_front(); return rx.size();
    }
    void onReceive(void (*)(int)) {}
    void onRequest(void (*)()) {}
    int available() { return rx.size(); }
    int read() { if (rx.empty()) return -1; const auto v = rx.front(); rx.pop_front(); return v; }
    size_t write(const uint8_t* values, size_t length) { tx.insert(tx.end(), values, values + length); return length; }
    size_t write(uint8_t value) { tx.push_back(value); return 1; }
};
extern FakeWire Wire;
