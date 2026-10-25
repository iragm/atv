#pragma once
#include <stdint.h>
#include <math.h>

namespace quad {

// Source is the present MOTOR-BRANCH high-side shunt, not a whole-battery shunt.
// Samples remain volatile. No estimate is shown unless a human supplies a
// starting state-of-charge reference for this power-on session.
class EnergyTracker {
 public:
  void update(uint32_t nowMs, float currentA, float voltageV, bool valid) {
    bool acceptable = valid && isfinite(currentA) && isfinite(voltageV) &&
                      voltageV >= 0.0f && currentA > -60.0f && currentA < 60.0f;
    if (acceptable) sawSample_ = true;
    if (acceptable && currentA > peakA_) peakA_ = currentA;
    if (initialized_) {
      const uint32_t delta = static_cast<uint32_t>(nowMs - previousMs_);
      // Large holes (offline sensors or a blocked main loop) are UNKNOWN load.
      if (previousValid_ && acceptable && delta > 0 && delta <= 1000) {
        const float secs = delta / 1000.0f;
        const float meanA = (positive(previousA_) + positive(currentA)) * 0.5f;
        const float meanW = (positive(previousA_) * previousV_ + positive(currentA) * voltageV) * 0.5f;
        const float ampSecs = meanA * secs;
        const float wattSecs = meanW * secs;
        ah_ += ampSecs / 3600.0f;
        wh_ += wattSecs / 3600.0f;
        const uint32_t second = nowMs / 1000U;
        Bucket& b = buckets_[second % 300U];
        if (!b.valid || b.second != second) b = {second, 0.0f, 0.0f, 0.0f, true};
        b.ampSeconds += ampSecs;
        b.wattSeconds += wattSecs;
        b.observedSeconds += secs;
      }
    }
    initialized_ = true;
    previousMs_ = nowMs;
    previousValid_ = acceptable;
    previousA_ = currentA;
    previousV_ = voltageV;
  }
  struct Recent { float amps, watts, observedSeconds; };
  Recent recent(uint32_t nowMs) const {
    float amps = 0, watts = 0, observed = 0;
    const uint32_t second = nowMs / 1000U;
    for (const auto& b : buckets_) {
      if (b.valid && static_cast<uint32_t>(second - b.second) < 300U) {
        amps += b.ampSeconds;
        watts += b.wattSeconds;
        observed += b.observedSeconds;
      }
    }
    return {observed > 0 ? amps / observed : NAN,
            observed > 0 ? watts / observed : NAN, observed};
  }
  bool hasData() const { return sawSample_; }
  float consumedAh() const { return ah_; }
  float consumedWh() const { return wh_; }
  float peakAmps() const { return sawSample_ ? peakA_ : NAN; }
  void setChargeReference(float percent) {
    referencePercent_ = isfinite(percent) && percent >= 0 && percent <= 100 ? percent : NAN;
    usedSinceReferenceAh_ = ah_;
  }
  float startingChargePercent() const { return referencePercent_; }
  float estimatedPercent(float capacityAh) const {
    if (!isfinite(referencePercent_) || !isfinite(capacityAh) || capacityAh <= 0) return NAN;
    const float soc = referencePercent_ - ((ah_ - usedSinceReferenceAh_) / capacityAh * 100.0f);
    return soc < 0 ? 0 : soc;
  }
  float minutesRemaining(uint32_t nowMs, float capacityAh) const {
    const Recent r = recent(nowMs);
    const float percent = estimatedPercent(capacityAh);
    if (!isfinite(percent) || !isfinite(r.amps) || r.observedSeconds < 30 || r.amps < 0.1f) return NAN;
    return (capacityAh * percent / 100.0f) / r.amps * 60.0f;
  }
 private:
  static float positive(float a) { return a > 0 ? a : 0; }
  struct Bucket {
    uint32_t second = 0;
    float ampSeconds = 0, wattSeconds = 0, observedSeconds = 0;
    bool valid = false;
  };
  Bucket buckets_[300] = {};
  bool sawSample_ = false;
  bool initialized_ = false;
  bool previousValid_ = false;
  uint32_t previousMs_ = 0;
  float previousA_ = 0, previousV_ = 0;
  float ah_ = 0, wh_ = 0, peakA_ = 0;
  float referencePercent_ = NAN;
  float usedSinceReferenceAh_ = 0;
};

// Threshold 0 disables either heuristic. Current is the motor's DC INPUT
// supply current, NOT winding current. No-load can be triggered by downhill
// travel, loss of mechanical load, or genuine wheel spin. It is not a slip sensor.
class LoadProtection {
 public:
  enum class Trip : uint8_t { None, HighCurrent, NoLoad };
  void evaluate(uint32_t now, bool driving, uint16_t duty, float amps, bool monitorValid,
                float maxAmps, uint16_t maxDelayMs,
                float minAmps, uint16_t minDelayMs) {
    if (trip_ != Trip::None) return;
    if (!driving || duty < 205 || !monitorValid || !isfinite(amps) || amps < -1.0f) {
      highPending_ = lowPending_ = false;
      return;
    }
    const bool high = maxAmps > 0 && amps >= maxAmps;
    const bool low = minAmps > 0 && amps <= minAmps;
    if (!high) highPending_ = false;
    else if (!highPending_) { highPending_ = true; highSince_ = now; }
    else if (static_cast<uint32_t>(now - highSince_) >= maxDelayMs) trip_ = Trip::HighCurrent;
    if (!low) lowPending_ = false;
    else if (!lowPending_) { lowPending_ = true; lowSince_ = now; }
    else if (static_cast<uint32_t>(now - lowSince_) >= minDelayMs) trip_ = Trip::NoLoad;
  }
  Trip trip() const { return trip_; }
  void resetAfterRelease() {
    trip_ = Trip::None;
    highPending_ = lowPending_ = false;
  }
 private:
  Trip trip_ = Trip::None;
  bool highPending_ = false, lowPending_ = false;
  uint32_t highSince_ = 0, lowSince_ = 0;
};
}
