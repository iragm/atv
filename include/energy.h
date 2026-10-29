#pragma once
#include <stdint.h>
#include <math.h>

namespace quad {

// Source is the present MOTOR-BRANCH high-side shunt, not a whole-battery shunt.
// Samples remain volatile. SOC comes from the separate voltage gauge.
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
  // Runtime uses voltage-derived state of charge, rated Ah, and recent motor draw.
  float minutesRemaining(uint32_t nowMs, float capacityAh, float chargePercent) const {
    const Recent r = recent(nowMs);
    if (!isfinite(chargePercent) || chargePercent < 0 || chargePercent > 100 ||
        !isfinite(capacityAh) || capacityAh <= 0 || !isfinite(r.amps) ||
        r.observedSeconds < 30 || r.amps < 0.1f) return NAN;
    return (capacityAh * chargePercent / 100.0f) / r.amps * 60.0f;
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
};

// Smoothed voltage-only percentage estimate; not coulomb counting.
// During motor load voltage sags, and charging raises voltage. The gauge is
// approximate even though the firmware can compute a precise percentage.
class BatteryGauge {
 public:
  void update(uint32_t nowMs, bool valid, float volts) {
    if (!valid || !isfinite(volts) || volts < 0.0f || volts > 25.0f) {
      hasVoltage_ = false; // Never expose a stale percentage when INA226 disappears.
      return;
    }
    if (!hasVoltage_) {
      filteredV_ = volts;
      hasVoltage_ = true;
    } else {
      const uint32_t elapsed = static_cast<uint32_t>(nowMs - previousMs_);
      // 10-second low-pass smoothing for PWM/supply noise.
      const float alpha = static_cast<float>(elapsed) / (10000.0f + elapsed);
      filteredV_ += alpha * (volts - filteredV_);
    }
    previousMs_ = nowMs;
  }
  float filteredVolts() const { return hasVoltage_ ? filteredV_ : NAN; }
  float percent(float emptyV, float fullV) const {
    if (!hasVoltage_ || !isfinite(emptyV) || !isfinite(fullV) ||
        fullV - emptyV < 0.2f) return NAN;
    float pct = 100.0f * (filteredV_ - emptyV) / (fullV - emptyV);
    return pct < 0 ? 0 : (pct > 100 ? 100 : pct);
  }
 private:
  bool hasVoltage_ = false;
  uint32_t previousMs_ = 0;
  float filteredV_ = NAN;
};

// Current is measured at the DC motor-branch input, not in motor windings.
// "Instant" means the first valid sampled reading, NOT an analog hardware trip.
// Trip thresholds default disabled until calibrated; there is no motion/RPM sensor.
class LoadProtection {
 public:
  enum class Trip : uint8_t { None, HighCurrent, NoLoad, EmergencyCurrent };
  struct Settings {
    float emergencyA = 0.0f;
    float stallA = 0.0f;
    uint16_t stallMs = 700;
    float noLoadBelowA = 0.0f;
    uint16_t noLoadMs = 1500;
    float derateA = 0.0f;
    uint16_t derateMs = 700;
    float derateMinimumV = 2.0f;
  };
  void evaluate(uint32_t now, bool driving, uint16_t duty, float amps, bool monitorValid,
                const Settings& limits) {
    const uint32_t elapsed = initialized_ ? static_cast<uint32_t>(now - lastMs_) : 0;
    lastMs_ = now;
    initialized_ = true;
    if (trip_ != Trip::None) return;
    if (!driving || duty == 0 || !monitorValid || !isfinite(amps) || amps < -1.0f) {
      highPending_ = lowPending_ = deratePending_ = false;
      derateCapV_ = 12.0f;
      return;
    }
    // A high-current spike must be caught even at small PWM startup duty.
    if (limits.emergencyA > 0 && amps >= limits.emergencyA) {
      trip_ = Trip::EmergencyCurrent;
      return;
    }
    // A timer must see the condition continuously. Non-monotonic millis() is
    // never used; subtraction intentionally accommodates uint32 wraparound.
    const bool high = limits.stallA > 0 && amps >= limits.stallA && duty >= 205;
    const bool low = limits.noLoadBelowA > 0 && amps <= limits.noLoadBelowA && duty >= 205;
    if (timedCondition(high, highPending_, highSince_, now, limits.stallMs)) {
      trip_ = Trip::HighCurrent;
      return;
    }
    if (timedCondition(low, lowPending_, lowSince_, now, limits.noLoadMs)) {
      trip_ = Trip::NoLoad;
      return;
    }
    // Derate only after a sustained overload. Below 90% of its setpoint, ramp
    // voltage cap back up slowly. At the threshold, hold to avoid oscillation.
    // Bound elapsed to avoid a huge jump after delayed polling / millis wrap.
    const float seconds = (elapsed < 250 ? elapsed : 250) / 1000.0f;
    const bool overload = limits.derateA > 0 && amps >= limits.derateA && duty >= 205;
    if (limits.derateA <= 0) {
      deratePending_ = false;
      derateCapV_ = 12.0f;
    } else if (timedCondition(overload, deratePending_, derateSince_, now, limits.derateMs)) {
      // Cap reduces 2 equivalent volts/s while overload persists.
      derateCapV_ -= 2.0f * seconds;
    } else if (amps < limits.derateA * 0.9f) {
      derateCapV_ += 1.0f * seconds;
    }
    if (derateCapV_ < limits.derateMinimumV) derateCapV_ = limits.derateMinimumV;
    if (derateCapV_ > 12.0f) derateCapV_ = 12.0f;
  }
  Trip trip() const { return trip_; }
  void resetDerating() { deratePending_ = false; derateCapV_ = 12.0f; }
  float derateCapVolts() const { return derateCapV_; }
  bool derating() const { return derateCapV_ < 11.99f; }
  void resetAfterRelease() {
    trip_ = Trip::None;
    highPending_ = lowPending_ = deratePending_ = false;
    derateCapV_ = 12.0f;
  }
 private:
  static bool timedCondition(bool condition, bool& pending, uint32_t& since,
                             uint32_t now, uint16_t durationMs) {
    if (!condition) { pending = false; return false; }
    if (!pending) { pending = true; since = now; return false; }
    return static_cast<uint32_t>(now - since) >= durationMs;
  }
  Trip trip_ = Trip::None;
  bool highPending_ = false, lowPending_ = false, deratePending_ = false;
  uint32_t highSince_ = 0, lowSince_ = 0, derateSince_ = 0;
  uint32_t lastMs_ = 0;
  bool initialized_ = false;
  float derateCapV_ = 12.0f;
};
}
