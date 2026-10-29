#include "settings.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
int main() {
  quad::Settings a{};
  assert(a.batteryFullV==13.2f && a.batteryEmptyV==11.8f);
  a.checksum=quad::settingsChecksum(a);
  assert(quad::validSettings(a));
  a.batteryEmptyV=13.2f;a.checksum=quad::settingsChecksum(a);
  assert(!quad::validSettings(a));
  a.batteryEmptyV=11.8f;a.batteryFullV=NAN;a.checksum=quad::settingsChecksum(a);
  assert(!quad::validSettings(a));
  a.batteryFullV=13.2f;a.checksum=quad::settingsChecksum(a);
  assert(quad::validSettings(a));
  quad::LegacySettingsV5 old{};
  old.magic=quad::SETTINGS_MAGIC;old.version=5;
  old.rampMs=3800;old.forwardV=7;old.reverseV=3;
  old.motorCutoffC=77;old.gearboxCutoffC=69;
  old.derateTripMs=700;old.derateMinimumV=2;
  old.stallTripMs=700;old.noLoadTripMs=1500;
  old.batteryCapacityAh=9;old.chargerCutoffV=13.2f;
  std::strcpy(old.apPassword,"existing-pass");
  std::strcpy(old.apSsid,"my-quad");
  std::strcpy(old.otaSsid,"homewifi");
  std::strcpy(old.otaPassword,"another-password");
  old.checksum=quad::legacyV5Checksum(old);
  quad::Settings upgraded;
  assert(quad::upgradeSettings(old,upgraded));
  assert(upgraded.version==6 && upgraded.rampMs==3800);
  assert(upgraded.batteryCapacityAh==9 && upgraded.chargerCutoffV==13.2f);
  assert(upgraded.batteryFullV==13.2f && upgraded.batteryEmptyV==11.8f);
  assert(std::strcmp(upgraded.apPassword,"existing-pass")==0);
  assert(std::strcmp(upgraded.otaPassword,"another-password")==0);
  old.checksum++;
  assert(!quad::upgradeSettings(old,upgraded));
  std::cout<<"battery settings migration tests passed\n";
}
