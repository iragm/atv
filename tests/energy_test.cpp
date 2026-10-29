#include "energy.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace quad;
int main() {
  EnergyTracker e;
  BatteryGauge gauge;
  assert(std::isnan(gauge.percent(11.8f,12.8f)));
  gauge.update(0,true,12.3f);
  assert(std::fabs(gauge.percent(11.8f,12.8f)-50.0f)<0.01f);
  assert(std::isnan(gauge.percent(12.8f,11.8f)));
  gauge.update(100,true,10.0f); // load spike is smoothed
  assert(gauge.percent(11.8f,12.8f)>45.0f);
  gauge.update(200,false,NAN); // missing sensor -> unknown
  assert(std::isnan(gauge.percent(11.8f,12.8f)));
  gauge.update(300,true,12.9f);
  assert(gauge.percent(11.8f,12.8f)==100.0f);
  gauge.update(400,true,11.0f);
  assert(gauge.percent(11.8f,12.8f)>90.0f);
  assert(!e.hasData() && std::isnan(e.peakAmps()));
  for (uint32_t t=0;t<=60000;t+=100) e.update(t,3.0f,12.0f,true);
  auto r=e.recent(60000);
  assert(r.observedSeconds>59.9f && r.amps>2.99f && r.amps<3.01f);
  assert(r.watts>35.9f && r.watts<36.1f);
  assert(e.consumedAh()>0.049f && e.consumedAh()<0.051f);
  assert(e.consumedWh()>0.59f && e.consumedWh()<0.61f);
  assert(e.minutesRemaining(60000,7.0f,75.0f)>104.0f);
  assert(std::isnan(e.minutesRemaining(60000,7.0f,NAN)));
  for (uint32_t t=60100;t<=180000;t+=100)e.update(t,3.0f,12.0f,true);
  // Time decay removes old samples from the 300-second average.
  for (uint32_t t=180100;t<=500000;t+=100)e.update(t,0.0f,12.0f,true);
  assert(e.recent(500000).amps<0.01f);
  assert(std::isnan(e.minutesRemaining(500000,7.0f,75.0f))); // zero recent draw -> unknown
  // Clamp and recovery after loss of sensor; new valid reading restarts gauge.
  gauge.update(500,true,10.0f);
  assert(gauge.percent(11.8f,12.8f)>=0.0f);
  // Sensor loss does not integrate a made-up current.
  float consumed=e.consumedAh();
  e.update(500100,NAN,NAN,false);e.update(500200,30.0f,12.0f,true);
  assert(std::fabs(e.consumedAh()-consumed)<0.00001f);
  LoadProtection p;
  LoadProtection::Settings cfg;
  cfg.stallA=10;cfg.stallMs=700;
  // Sustained high current: continuous readings only, no trip on a spike.
  p.evaluate(100,true,512,12,true,cfg);
  p.evaluate(500,true,512,12,true,cfg);
  assert(p.trip()==LoadProtection::Trip::None);
  p.evaluate(801,true,512,12,true,cfg);
  assert(p.trip()==LoadProtection::Trip::HighCurrent);
  p.evaluate(900,false,0,0,true,cfg);
  assert(p.trip()==LoadProtection::Trip::HighCurrent);
  p.resetAfterRelease();
  p.evaluate(1200,true,512,12,true,cfg);
  p.evaluate(1500,true,512,6,true,cfg); // interruptions clear the timer
  p.evaluate(1900,true,512,12,true,cfg);
  assert(p.trip()==LoadProtection::Trip::None);
  p.resetAfterRelease();
  cfg.stallA=0;cfg.noLoadBelowA=0.5f;cfg.noLoadMs=1500;
  p.evaluate(2000,true,500,0.1f,true,cfg);
  p.evaluate(3400,true,500,0.1f,true,cfg);
  assert(p.trip()==LoadProtection::Trip::None);
  p.evaluate(3501,true,500,0.1f,true,cfg);
  assert(p.trip()==LoadProtection::Trip::NoLoad);
  p.resetAfterRelease();
  cfg.noLoadBelowA=0;cfg.stallA=10;
  p.evaluate(4000,true,100,40,true,cfg); // timed stall ignores tiny startup duty
  p.evaluate(5000,true,100,40,true,cfg);
  assert(p.trip()==LoadProtection::Trip::None);
  p.evaluate(5200,true,512,NAN,false,cfg); // missing INA leaves protection inactive
  assert(p.trip()==LoadProtection::Trip::None);
  // Emergency threshold trips on the FIRST valid measurement, including tiny PWM.
  cfg.emergencyA=25;
  p.evaluate(5300,true,12,24.9f,true,cfg);
  assert(p.trip()==LoadProtection::Trip::None);
  p.evaluate(5325,true,12,25.1f,true,cfg);
  assert(p.trip()==LoadProtection::Trip::EmergencyCurrent);
  p.resetAfterRelease();
  p.evaluate(5350,true,12,NAN,false,cfg);
  assert(p.trip()==LoadProtection::Trip::None);
  // Independent progressive derating, only after configured sustained overload.
  cfg.emergencyA=0;cfg.stallA=0;cfg.derateA=7;cfg.derateMs=500;cfg.derateMinimumV=3;
  for(uint32_t t=6000;t<=10500;t+=25)p.evaluate(t,true,512,8,true,cfg);
  assert(p.trip()==LoadProtection::Trip::None);
  assert(p.derateCapVolts()<=4.1f && p.derateCapVolts()>=3.0f);
  for(uint32_t t=10525;t<=14000;t+=25)p.evaluate(t,true,512,8,true,cfg);
  assert(p.derateCapVolts()>=3 && p.derateCapVolts()<3.01f);
  for(uint32_t t=14025;t<=17500;t+=25)p.evaluate(t,true,512,4,true,cfg);
  assert(p.derateCapVolts()>6.0f); // recovery after dropping below 90% threshold
  p.evaluate(17525,false,0,0,true,cfg);
  assert(!p.derating());
  // Wraparound-safe elapsed time.
  LoadProtection wraps;
  cfg.stallA=10;cfg.stallMs=200;cfg.derateA=0;
  wraps.evaluate(0xffffffaaU,true,512,12,true,cfg);
  wraps.evaluate(0x00000080U,true,512,12,true,cfg);
  assert(wraps.trip()==LoadProtection::Trip::HighCurrent);
  std::cout << "energy and protection tests passed\n";
}
