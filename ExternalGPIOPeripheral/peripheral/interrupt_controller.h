#ifndef INTERRUPT_CONTROLLER_H
#define INTERRUPT_CONTROLLER_H

#include <Arduino.h>

class InterruptController {
  public:
    InterruptController();
    void begin();
    void trigger();
    void clear();
    bool hasPendingInterrupt() const;
    void setInterruptEnabled(bool enabled);

  private:
    bool pending_;
    bool enabled_;
};

#endif
