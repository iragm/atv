#include "settings.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
int main() {
  quad::Settings a{};
  assert(a.batteryFullV==13.2f && a.batteryEmptyV==11.8f);
  assert(a.chargerCutoffV==13.9f && a.directionMode==0 && a.version==7);
  a.checksum=quad::settingsChecksum(a);
  assert(quad::validSettings(a));
  a.batteryEmptyV=13.2f;a.checksum=quad::settingsChecksum(a);
  assert(!quad::validSettings(a));
  a.batteryEmptyV=11.8f;a.batteryFullV=NAN;a.checksum=quad::settingsChecksum(a);
  assert(!quad::validSettings(a));
  a.batteryFullV=13.2f;a.checksum=quad::settingsChecksum(a);
  assert(quad::validSettings(a));
  a.directionMode=1;a.checksum=quad::settingsChecksum(a);assert(quad::validSettings(a));
  a.directionMode=2;a.checksum=quad::settingsChecksum(a);assert(!quad::validSettings(a));
  a.directionMode=0;a.checksum=quad::settingsChecksum(a);
  quad::LegacySettingsV6 old6{};
  old6.magic=quad::SETTINGS_MAGIC;old6.version=6;
  old6.rampMs=3300;old6.forwardV=7;old6.reverseV=3;
  old6.motorCutoffC=75;old6.gearboxCutoffC=70;
  old6.derateTripMs=700;old6.derateMinimumV=2;
  old6.stallTripMs=700;old6.noLoadTripMs=1500;
  old6.batteryCapacityAh=9;old6.batteryFullV=13.2f;
  old6.batteryEmptyV=11.8f;old6.chargerCutoffV=13.9f;
  std::strcpy(old6.apPassword,"original-pass");
  std::strcpy(old6.otaSsid,"homewifi");
  std::strcpy(old6.otaPassword,"keep-ota-password");
  old6.checksum=quad::legacyV6Checksum(old6);
  quad::Settings from6;
  assert(quad::upgradeSettings(old6,from6));
  assert(from6.version==7 && from6.directionMode==0 && from6.rampMs==3300);
  assert(std::strcmp(from6.otaPassword,"keep-ota-password")==0);
  old6.checksum++;assert(!quad::upgradeSettings(old6,from6));
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
  assert(upgraded.version==7 && upgraded.rampMs==3800 && upgraded.directionMode==0);
  assert(upgraded.batteryCapacityAh==9 && upgraded.chargerCutoffV==13.2f);
  assert(upgraded.batteryFullV==13.2f && upgraded.batteryEmptyV==11.8f);
  assert(std::strcmp(upgraded.apPassword,"existing-pass")==0);
  assert(std::strcmp(upgraded.otaPassword,"another-password")==0);
  old.checksum++;
  assert(!quad::upgradeSettings(old,upgraded));
  std::cout<<"battery settings migration tests passed\n";
}
