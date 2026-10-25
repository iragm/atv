#pragma once
#include <stdint.h>

namespace quad {
enum class Direction : uint8_t { Invalid, Forward, Reverse };
enum class State : uint8_t { WaitRelease, Ready, Ramp, ReleaseHold, Charging, Fault };
enum class Fault : uint8_t { None, Direction, Sensor, Overcurrent, NoLoad };
enum class Source : uint8_t { None, Pedal, Wifi };

struct Limits {
  uint16_t rampMs = 2000;
  uint16_t graceMs = 2000;
  float maxForwardV = 6.0f;
  float maxReverseV = 4.0f;
};
struct Input {
  bool physicalPressed = false;
  bool wifiPressed = false;
  bool charger = false;
  Direction physicalDirection = Direction::Invalid;
  Direction wifiDirection = Direction::Invalid;
  bool sensorFault = false;
  bool overcurrent = false;
  bool noLoad = false;
  bool thermalLimit = false;
  float supplyVolts = 12.0f; // nominal fallback if INA226 is missing
};
struct Output {
  Direction direction = Direction::Invalid;
  State state = State::WaitRelease;
  Fault fault = Fault::None;
  Source source = Source::None;
  uint16_t duty = 0;
  bool enabled = false;
  bool temperatureLimited = false;
};

// Platform-independent controller, tested on the CI host without Arduino.
class Controller {
 public:
  explicit Controller(const Limits& limits = Limits{}) : limits_(limits) {}
  void setLimits(const Limits& limits) { limits_ = limits; }
  Output step(uint32_t now, const Input& in) {
    // Center OFF removes switched power, even if Wi-Fi requests motion.
    if (in.charger || in.physicalDirection == Direction::Invalid || in.sensorFault || in.overcurrent || in.noLoad) {
      if (in.overcurrent) fault_ = Fault::Overcurrent;
      else if (in.noLoad) fault_ = Fault::NoLoad;
      else if (in.sensorFault) fault_ = Fault::Sensor;
      else if (!in.charger && in.physicalDirection == Direction::Invalid) fault_ = Fault::Direction;
      state_ = in.charger ? State::Charging : State::Fault;
      armed_ = false;
      running_ = false;
      source_ = Source::None;
      lastDuty_ = 0;
      return stopped();
    }

    Source requestedSource = Source::None;
    Direction requestedDirection = Direction::Invalid;
    if (in.physicalPressed) {
      requestedSource = Source::Pedal;
      requestedDirection = in.physicalDirection;
    } else if (in.wifiPressed && in.wifiDirection != Direction::Invalid) {
      requestedSource = Source::Wifi;
      requestedDirection = in.wifiDirection;
    }

    // A physical pedal release continues at the last duty for at most graceMs.
    // Direction switching never inherits the grace period.
    if (requestedSource == Source::None && running_ && source_ == Source::Pedal &&
        direction_ == in.physicalDirection) {
      if (!graceActive_) { graceActive_ = true; graceStart_ = now; }
      if (static_cast<uint32_t>(now - graceStart_) < limits_.graceMs) {
        state_ = State::ReleaseHold;
        const uint16_t effectiveDuty = limitedDuty(lastDuty_, direction_, in);
        return {direction_, state_, fault_, source_, effectiveDuty, effectiveDuty > 0, in.thermalLimit};
      }
    }

    if (requestedSource == Source::None) {
      armed_ = true;
      running_ = false;
      graceActive_ = false;
      source_ = Source::None;
      lastDuty_ = 0;
      state_ = State::Ready;
      fault_ = Fault::None;
      return stopped();
    }

    // No hot source or direction handoff. Both must be released before rearming.
    if (running_ && (requestedSource != source_ || requestedDirection != direction_)) {
      running_ = false;
      armed_ = false;
      lastDuty_ = 0;
      source_ = requestedSource;
      direction_ = requestedDirection;
      state_ = State::WaitRelease;
      return stopped();
    }
    if (!armed_) {
      state_ = State::WaitRelease;
      return stopped();
    }
    if (!running_) {
      running_ = true;
      rampStart_ = now;
      lastDuty_ = 0;
    } else if (graceActive_) {
      // Resume the previous ramp without a power discontinuity.
      rampStart_ += static_cast<uint32_t>(now - graceStart_);
    }
    graceActive_ = false;
    source_ = requestedSource;
    direction_ = requestedDirection;
    fault_ = Fault::None;
    state_ = State::Ramp;
    const uint16_t cap = maximumDuty(direction_, in);
    const uint32_t ramp = limits_.rampMs ? limits_.rampMs : 1;
    uint32_t elapsed = static_cast<uint32_t>(now - rampStart_);
    if (elapsed > ramp) elapsed = ramp;
    lastDuty_ = static_cast<uint16_t>(cap * (static_cast<float>(elapsed) / ramp));
    return {direction_, state_, fault_, source_, lastDuty_, lastDuty_ > 0, in.thermalLimit};
  }

 private:
  uint16_t maximumDuty(Direction d, const Input& in) const {
    const float setpoint = d == Direction::Forward ? limits_.maxForwardV : limits_.maxReverseV;
    const float voltage = in.thermalLimit && setpoint > 5.0f ? 5.0f : setpoint;
    const float supply = (in.supplyVolts >= 5.0f && in.supplyVolts <= 17.0f) ? in.supplyVolts : 12.0f;
    float cap = 1023.0f * voltage / supply;
    if (cap > 1023.0f) cap = 1023.0f;
    if (cap < 0.0f) cap = 0.0f;
    return static_cast<uint16_t>(cap);
  }
  uint16_t limitedDuty(uint16_t duty, Direction d, const Input& in) const {
    const uint16_t cap = maximumDuty(d, in);
    return duty < cap ? duty : cap;
  }
  Output stopped() const { return {direction_, state_, fault_, source_, 0, false, false}; }
  Limits limits_;
  bool armed_ = false;
  bool running_ = false;
  bool graceActive_ = false;
  uint32_t rampStart_ = 0;
  uint32_t graceStart_ = 0;
  uint16_t lastDuty_ = 0;
  Source source_ = Source::None;
  Direction direction_ = Direction::Invalid;
  State state_ = State::WaitRelease;
  Fault fault_ = Fault::None;
};
inline Direction decodeDirection(uint16_t adc, uint16_t fMin, uint16_t fMax,
                                 uint16_t rMin, uint16_t rMax) {
  if (adc >= fMin && adc <= fMax) return Direction::Forward;
  if (adc >= rMin && adc <= rMax) return Direction::Reverse;
  return Direction::Invalid;
}
} // namespace quad
