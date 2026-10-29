#pragma once
#include <stdint.h>
#include <math.h>
#include <string.h>

namespace quad {
constexpr uint32_t SETTINGS_MAGIC = 0x51554144UL;
constexpr uint16_t SETTINGS_VERSION = 5;
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
  float chargerCutoffV = 13.0f;
  char apSsid[33] = {}; // empty means QUAD-<chip-id>
  char apPassword[64] = "quadsetup26";
  char otaSsid[33] = {}; // only used during OTA
  char otaPassword[64] = {};
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
  mix(&s.chargerCutoffV, sizeof(s.chargerCutoffV));
  mix(s.apSsid, sizeof(s.apSsid)); mix(s.apPassword, sizeof(s.apPassword));
  mix(s.otaSsid, sizeof(s.otaSsid)); mix(s.otaPassword, sizeof(s.otaPassword));
  return hash;
}
inline bool validCredential(const char* str, unsigned cap, unsigned minLen, unsigned maxLen, bool allowEmpty) {
  const void* ending = memchr(str, 0, cap);
  if (!ending) return false;
  const unsigned len = static_cast<const char*>(ending) - str;
  if (!len) return allowEmpty;
  if (len < minLen || len > maxLen) return false;
  for (unsigned i=0;i<len;i++) if (static_cast<unsigned char>(str[i]) < 0x20 || static_cast<unsigned char>(str[i]) == 0x7f) return false;
  return true;
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
      !(s.derateTripA > 0 && s.stallTripA > 0 && s.derateTripA >= s.stallTripA) &&
      isfinite(s.chargerCutoffV) && s.chargerCutoffV >= 12.5f && s.chargerCutoffV <= 15.5f &&
      validCredential(s.apSsid,sizeof(s.apSsid),1,32,true) &&
      validCredential(s.apPassword,sizeof(s.apPassword),8,63,false) &&
      validCredential(s.otaSsid,sizeof(s.otaSsid),1,32,true) &&
      validCredential(s.otaPassword,sizeof(s.otaPassword),8,63,true) &&
      (!s.otaSsid[0] || s.otaPassword[0]);
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

namespace quad {
// Exact previous v4 EEPROM layout. Preserve configuration and calibrations.
struct LegacySettingsV4 {
  uint32_t magic;
  uint16_t version, rampMs;
  float forwardV, reverseV, motorCutoffC, gearboxCutoffC;
  uint8_t motorRom[8], gearboxRom[8];
  float emergencyTripA, derateTripA;
  uint16_t derateTripMs;
  float derateMinimumV;
  float stallTripA;
  uint16_t stallTripMs;
  float noLoadBelowA;
  uint16_t noLoadTripMs;
  float batteryCapacityAh;
  uint32_t checksum;
};
inline uint32_t legacyV4Checksum(const LegacySettingsV4& s) {
  uint32_t hash=2166136261UL;
  const auto mix=[&hash](const void* p, unsigned n) {
    const uint8_t* b=static_cast<const uint8_t*>(p);
    for(unsigned i=0;i<n;i++) { hash ^= b[i]; hash *= 16777619UL; }
  };
  mix(&s.magic,sizeof(s.magic));mix(&s.version,sizeof(s.version));mix(&s.rampMs,sizeof(s.rampMs));
  mix(&s.forwardV,sizeof(s.forwardV));mix(&s.reverseV,sizeof(s.reverseV));
  mix(&s.motorCutoffC,sizeof(s.motorCutoffC));mix(&s.gearboxCutoffC,sizeof(s.gearboxCutoffC));
  mix(s.motorRom,sizeof(s.motorRom));mix(s.gearboxRom,sizeof(s.gearboxRom));
  mix(&s.emergencyTripA,sizeof(s.emergencyTripA));mix(&s.derateTripA,sizeof(s.derateTripA));
  mix(&s.derateTripMs,sizeof(s.derateTripMs));mix(&s.derateMinimumV,sizeof(s.derateMinimumV));
  mix(&s.stallTripA,sizeof(s.stallTripA));mix(&s.stallTripMs,sizeof(s.stallTripMs));
  mix(&s.noLoadBelowA,sizeof(s.noLoadBelowA));mix(&s.noLoadTripMs,sizeof(s.noLoadTripMs));
  mix(&s.batteryCapacityAh,sizeof(s.batteryCapacityAh));
  return hash;
}
inline bool upgradeSettings(const LegacySettingsV4& old, Settings& out) {
  if(old.magic!=SETTINGS_MAGIC || old.version!=4 || old.checksum!=legacyV4Checksum(old))return false;
  out=Settings{};
  out.rampMs=old.rampMs;out.forwardV=old.forwardV;out.reverseV=old.reverseV;
  out.motorCutoffC=old.motorCutoffC;out.gearboxCutoffC=old.gearboxCutoffC;
  memcpy(out.motorRom,old.motorRom,8);memcpy(out.gearboxRom,old.gearboxRom,8);
  out.emergencyTripA=old.emergencyTripA;out.derateTripA=old.derateTripA;
  out.derateTripMs=old.derateTripMs;out.derateMinimumV=old.derateMinimumV;
  out.stallTripA=old.stallTripA;out.stallTripMs=old.stallTripMs;
  out.noLoadBelowA=old.noLoadBelowA;out.noLoadTripMs=old.noLoadTripMs;
  out.batteryCapacityAh=old.batteryCapacityAh;
  out.checksum=settingsChecksum(out);
  return validSettings(out);
}
}
