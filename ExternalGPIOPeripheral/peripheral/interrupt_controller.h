#pragma once
#include <Arduino.h>
class InterruptController {
public:
    void begin();
    void setPending(bool pending);
    bool hasPendingInterrupt() const;
private:
    volatile bool pending_ = false;
};
