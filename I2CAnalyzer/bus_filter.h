#pragma once
#include "bus_sampler.h"
// Observed-state filter. It cannot recover unseen physical edges.
class BusFilter {
public:
    void reset(uint8_t minimumUs) { threshold_ = minimumUs; pending_ = stable_ = BusState(); }
    bool push(const BusState& state, BusState& accepted) {
        if (!threshold_) { accepted = state; return true; }
        const bool ready = settle(state.atUs, accepted);
        if (different(state, pending_)) pending_ = state;
        return ready;
    }
    bool settle(uint32_t now, BusState& accepted) {
        if (!threshold_ || !different(pending_, stable_) || uint32_t(now - pending_.atUs) < threshold_) return false;
        stable_ = pending_; accepted = pending_; return true;
    }
private:
    static bool different(const BusState& a, const BusState& b) { return a.sda != b.sda || a.scl != b.scl; }
    BusState pending_, stable_;
    uint8_t threshold_ = 0;
};
