#pragma once
#include <stdint.h>
#include <math.h>
#include <string.h>

namespace quad {
constexpr uint32_t SETTINGS_MAGIC = 0x51554144UL;
constexpr uint16_t SETTINGS_VERSION = 4;
struct Settings {
  uint32_t magic = SETTINGS_MAGIC;
  uint16_t version = SETTINGS_VERSION;
  uint16_t rampMs = 2000;
  float forwardV = 6.0f;
  float reverseV = 4.0f;
  float motorCutoffC = 75.0f;
  float gearboxCutoffC = 70.0f;
  uint8_t motorRom[8] = {};
  uint8_t gearboxRom[8] = {};
  float emergencyTripA = 0.0f;  // first sampled current above threshold -> latch
  float derateTripA = 0.0f;     // sustained-overload PWM reduction
  uint16_t derateTripMs = 700;
  float derateMinimumV = 2.0f;
  float stallTripA = 0.0f;         // disabled until measured/calibrated
  uint16_t stallTripMs = 700;
  float noLoadBelowA = 0.0f;       // optional heuristic, disabled by default
  uint16_t noLoadTripMs = 1500;
  float batteryCapacityAh = 7.0f;  // placeholder: enter pack label/rated usable Ah
  uint32_t checksum = 0;
};
inline uint32_t settingsChecksum(const Settings& s) {
  // FNV1a over named fields, so padding is irrelevant.
  uint32_t hash = 2166136261UL;
  const auto mix = [&hash](const void* ptr, unsigned len) {
    const uint8_t* b = static_cast<const uint8_t*>(ptr);
    for (unsigned i = 0; i < len; ++i) { hash ^= b[i]; hash *= 16777619UL; }
  };
  mix(&s.magic, sizeof(s.magic)); mix(&s.version, sizeof(s.version));
  mix(&s.rampMs, sizeof(s.rampMs));
  mix(&s.forwardV, sizeof(s.forwardV)); mix(&s.reverseV, sizeof(s.reverseV));
  mix(&s.motorCutoffC, sizeof(s.motorCutoffC));
  mix(&s.gearboxCutoffC, sizeof(s.gearboxCutoffC));
  mix(s.motorRom, sizeof(s.motorRom)); mix(s.gearboxRom, sizeof(s.gearboxRom));
  mix(&s.emergencyTripA, sizeof(s.emergencyTripA));
  mix(&s.derateTripA, sizeof(s.derateTripA));
  mix(&s.derateTripMs, sizeof(s.derateTripMs));
  mix(&s.derateMinimumV, sizeof(s.derateMinimumV));
  mix(&s.stallTripA, sizeof(s.stallTripA)); mix(&s.stallTripMs, sizeof(s.stallTripMs));
  mix(&s.noLoadBelowA, sizeof(s.noLoadBelowA)); mix(&s.noLoadTripMs, sizeof(s.noLoadTripMs));
  mix(&s.batteryCapacityAh, sizeof(s.batteryCapacityAh));
  return hash;
}
inline bool validSettings(const Settings& s) {
  return s.magic == SETTINGS_MAGIC && s.version == SETTINGS_VERSION &&
      s.checksum == settingsChecksum(s) && s.rampMs >= 100 && s.rampMs <= 30000 &&
      isfinite(s.forwardV) && s.forwardV >= 0.5f && s.forwardV <= 12.0f &&
      isfinite(s.reverseV) && s.reverseV >= 0.5f && s.reverseV <= 12.0f &&
      isfinite(s.motorCutoffC) && s.motorCutoffC >= 20.0f && s.motorCutoffC <= 120.0f &&
      isfinite(s.gearboxCutoffC) && s.gearboxCutoffC >= 20.0f && s.gearboxCutoffC <= 120.0f &&
      isfinite(s.emergencyTripA) && s.emergencyTripA >= 0 && s.emergencyTripA <= 50 &&
      isfinite(s.derateTripA) && s.derateTripA >= 0 && s.derateTripA <= 50 &&
      s.derateTripMs >= 200 && s.derateTripMs <= 5000 &&
      isfinite(s.derateMinimumV) && s.derateMinimumV >= 0.5f && s.derateMinimumV <= 12.0f &&
      isfinite(s.stallTripA) && s.stallTripA >= 0 && s.stallTripA <= 50 &&
      s.stallTripMs >= 200 && s.stallTripMs <= 5000 &&
      isfinite(s.noLoadBelowA) && s.noLoadBelowA >= 0 && s.noLoadBelowA <= 20 &&
      s.noLoadTripMs >= 500 && s.noLoadTripMs <= 10000 &&
      isfinite(s.batteryCapacityAh) && s.batteryCapacityAh >= 1 && s.batteryCapacityAh <= 100 &&
      !(s.stallTripA > 0 && s.noLoadBelowA > 0 && s.noLoadBelowA >= s.stallTripA) &&
      !(s.derateTripA > 0 && s.emergencyTripA > 0 && s.derateTripA >= s.emergencyTripA) &&
      !(s.stallTripA > 0 && s.emergencyTripA > 0 && s.stallTripA >= s.emergencyTripA) &&
      !(s.derateTripA > 0 && s.stallTripA > 0 && s.derateTripA >= s.stallTripA);
}
inline bool emptyRom(const uint8_t rom[8]) {
  for (int i = 0; i < 8; ++i) if (rom[i]) return false;
  return true;
}
} // namespace quad

namespace quad {
struct LegacySettingsV2 {
  uint32_t magic;
  uint16_t version, rampMs;
  float forwardV, reverseV, motorCutoffC, gearboxCutoffC;
  uint8_t motorRom[8], gearboxRom[8];
  uint32_t checksum;
};
inline uint32_t legacyV2Checksum(const LegacySettingsV2& s) {
  uint32_t hash = 2166136261UL;
  const auto mix = [&hash](const void* ptr, unsigned len) {
    const uint8_t* b = static_cast<const uint8_t*>(ptr);
    for(unsigned i=0;i<len;++i) { hash ^= b[i];hash *= 16777619UL; }
  };
  mix(&s.magic,sizeof(s.magic));mix(&s.version,sizeof(s.version));mix(&s.rampMs,sizeof(s.rampMs));
  mix(&s.forwardV,sizeof(s.forwardV));mix(&s.reverseV,sizeof(s.reverseV));
  mix(&s.motorCutoffC,sizeof(s.motorCutoffC));mix(&s.gearboxCutoffC,sizeof(s.gearboxCutoffC));
  mix(s.motorRom,sizeof(s.motorRom));mix(s.gearboxRom,sizeof(s.gearboxRom));
  return hash;
}
inline bool upgradeSettings(const LegacySettingsV2& old, Settings& result) {
  if(old.magic != SETTINGS_MAGIC || old.version != 2 || old.checksum != legacyV2Checksum(old)) return false;
  result = Settings{};
  result.rampMs=old.rampMs;result.forwardV=old.forwardV;result.reverseV=old.reverseV;
  result.motorCutoffC=old.motorCutoffC;result.gearboxCutoffC=old.gearboxCutoffC;
  memcpy(result.motorRom,old.motorRom,8);memcpy(result.gearboxRom,old.gearboxRom,8);
  result.checksum=settingsChecksum(result);
  return validSettings(result);
}
}

namespace quad {
// Byte layout of v3 EEPROM, before emergency cutoff and derating were added.
struct LegacySettingsV3 {
  uint32_t magic;
  uint16_t version, rampMs;
  float forwardV, reverseV, motorCutoffC, gearboxCutoffC;
  uint8_t motorRom[8], gearboxRom[8];
  float stallTripA;
  uint16_t stallTripMs;
  float noLoadBelowA;
  uint16_t noLoadTripMs;
  float batteryCapacityAh;
  uint32_t checksum;
};
inline uint32_t legacyV3Checksum(const LegacySettingsV3& s) {
  uint32_t hash = 2166136261UL;
  const auto mix = [&hash](const void* ptr, unsigned len) {
    const uint8_t* b = static_cast<const uint8_t*>(ptr);
    for(unsigned i=0;i<len;++i) { hash ^= b[i];hash *= 16777619UL; }
  };
  mix(&s.magic,sizeof(s.magic)); mix(&s.version,sizeof(s.version)); mix(&s.rampMs,sizeof(s.rampMs));
  mix(&s.forwardV,sizeof(s.forwardV)); mix(&s.reverseV,sizeof(s.reverseV));
  mix(&s.motorCutoffC,sizeof(s.motorCutoffC)); mix(&s.gearboxCutoffC,sizeof(s.gearboxCutoffC));
  mix(s.motorRom,sizeof(s.motorRom)); mix(s.gearboxRom,sizeof(s.gearboxRom));
  mix(&s.stallTripA,sizeof(s.stallTripA)); mix(&s.stallTripMs,sizeof(s.stallTripMs));
  mix(&s.noLoadBelowA,sizeof(s.noLoadBelowA)); mix(&s.noLoadTripMs,sizeof(s.noLoadTripMs));
  mix(&s.batteryCapacityAh,sizeof(s.batteryCapacityAh));
  return hash;
}
inline bool upgradeSettings(const LegacySettingsV3& old, Settings& result) {
  if(old.magic != SETTINGS_MAGIC || old.version != 3 || old.checksum != legacyV3Checksum(old)) return false;
  result=Settings{};
  result.rampMs=old.rampMs;result.forwardV=old.forwardV;result.reverseV=old.reverseV;
  result.motorCutoffC=old.motorCutoffC;result.gearboxCutoffC=old.gearboxCutoffC;
  memcpy(result.motorRom,old.motorRom,8);memcpy(result.gearboxRom,old.gearboxRom,8);
  result.stallTripA=old.stallTripA;result.stallTripMs=old.stallTripMs;
  result.noLoadBelowA=old.noLoadBelowA;result.noLoadTripMs=old.noLoadTripMs;
  result.batteryCapacityAh=old.batteryCapacityAh;
  result.checksum=settingsChecksum(result);
  return validSettings(result);
}
}
