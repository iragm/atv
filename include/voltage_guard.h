#pragma once
#include <stdint.h>
#include <math.h>
namespace quad {
// Software inference of charging, NOT plug detection. Disengages immediately on
// a high valid bus-voltage sample and rearms only after stable low voltage.
class VoltageGuard {
 public:
  void step(uint32_t now, bool valid, float volts, float cutoffV) {
    if (!valid || !isfinite(volts) || volts < 0 || volts > 25) {
      belowPending_ = false; // A missing sample cannot clear a prior inhibit.
      return;
    }
    if (volts > cutoffV) { inhibited_ = true; belowPending_ = false; return; }
    if (!inhibited_) return;
    if (volts > cutoffV - 0.2f) { belowPending_ = false; return; }
    if (!belowPending_) { belowPending_ = true; belowSince_ = now; }
    else if (static_cast<uint32_t>(now - belowSince_) >= 5000) {
      inhibited_ = false;
      belowPending_ = false;
    }
  }
  bool inhibited() const { return inhibited_; }
 private:
  bool inhibited_ = false, belowPending_ = false;
  uint32_t belowSince_ = 0;
};
}
