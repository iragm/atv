#include "control.h"
#include "settings.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace quad;
static Input forward() { Input i; i.physicalDirection=Direction::Forward; i.supplyVolts=12.0f; return i; }
int main() {
  Settings cfg;
  assert(!validSettings(cfg)); // needs checksum
  cfg.checksum=settingsChecksum(cfg);
  assert(validSettings(cfg));
  cfg.forwardV=13;
  assert(!validSettings(cfg));
  Settings limits;
  limits.stallTripA=5.0f;limits.noLoadBelowA=6.0f;
  limits.checksum=settingsChecksum(limits);
  assert(!validSettings(limits));
  LegacySettingsV2 old{};
  old.magic=SETTINGS_MAGIC;old.version=2;old.rampMs=5000;
  old.forwardV=7.0f;old.reverseV=3.0f;old.motorCutoffC=66.0f;old.gearboxCutoffC=60.0f;
  old.checksum=legacyV2Checksum(old);
  Settings migrated;
  assert(upgradeSettings(old,migrated));
  assert(validSettings(migrated) && migrated.rampMs==5000 && migrated.forwardV==7.0f);
  assert(migrated.stallTripA==0.0f && migrated.noLoadBelowA==0.0f);
  old.checksum^=1;
  assert(!upgradeSettings(old,migrated));
  Controller c;
  Input i=forward();
  i.physicalPressed=true;
  assert(c.step(0,i).state==State::WaitRelease); // pressed at startup
  i.physicalPressed=false;
  assert(c.step(10,i).state==State::Ready);
  i.physicalPressed=true;
  c.step(20,i);
  auto mid=c.step(1020,i);
  assert(mid.duty>=255 && mid.duty<=257); // 6V equivalent on 12V @ 50% ramp
  assert(c.step(2020,i).duty>=510);
  i.physicalPressed=false;
  auto hold=c.step(2030,i);
  assert(hold.state==State::ReleaseHold && hold.enabled);
  assert(c.step(4029,i).enabled);
  i.thermalLimit=true;
  assert(c.step(4029,i).duty<=427); // temp cutoff must work during grace
  i.thermalLimit=false;
  assert(!c.step(4030,i).enabled);
  i.physicalPressed=true;
  c.step(4050,i);
  assert(c.step(6050,i).duty>=510);
  i.thermalLimit=true;
  assert(c.step(6060,i).duty<=427); // 5V/12V limit
  i.charger=true;
  assert(!c.step(6070,i).enabled && c.step(6070,i).state==State::Charging);
  i.charger=false;
  assert(c.step(6080,i).state==State::WaitRelease);
  i.physicalPressed=false;
  assert(c.step(6090,i).state==State::Ready);
  i.wifiPressed=true;
  i.wifiDirection=Direction::Reverse;
  c.step(6100,i);
  assert(c.step(8100,i).duty>=340); // 4V reverse
  i.wifiPressed=false;
  assert(!c.step(8110,i).enabled); // WiFi has no grace
  i.wifiPressed=true;
  i.wifiDirection=Direction::Forward;
  c.step(8200,i);
  assert(c.step(10200,i).direction==Direction::Forward);
  i.physicalDirection=Direction::Invalid;
  assert(!c.step(10210,i).enabled); // hardware switch center-off
  assert(decodeDirection(85,45,145,275,410)==Direction::Forward);
  assert(decodeDirection(340,45,145,275,410)==Direction::Reverse);
  assert(decodeDirection(500,45,145,275,410)==Direction::Invalid);
  std::cout << "controller and settings tests passed\n";
}
