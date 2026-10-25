#include "energy.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace quad;
int main() {
  EnergyTracker e;
  assert(std::isnan(e.estimatedPercent(7)));
  assert(!e.hasData() && std::isnan(e.peakAmps()));
  for (uint32_t t=0;t<=60000;t+=100) e.update(t,3.0f,12.0f,true);
  auto r=e.recent(60000);
  assert(r.observedSeconds>59.9f && r.amps>2.99f && r.amps<3.01f);
  assert(r.watts>35.9f && r.watts<36.1f);
  assert(e.consumedAh()>0.049f && e.consumedAh()<0.051f);
  assert(e.consumedWh()>0.59f && e.consumedWh()<0.61f);
  e.setChargeReference(75.0f);
  assert(e.estimatedPercent(7.0f)>74.99f);
  assert(e.minutesRemaining(60000,7.0f)>104.0f);
  for (uint32_t t=60100;t<=180000;t+=100)e.update(t,3.0f,12.0f,true);
  assert(e.estimatedPercent(7.0f)<75);
  // Time decay removes old samples from the 300-second average.
  for (uint32_t t=180100;t<=500000;t+=100)e.update(t,0.0f,12.0f,true);
  assert(e.recent(500000).amps<0.01f);
  assert(std::isnan(e.minutesRemaining(500000,7.0f))); // zero recent draw -> unknown
  // Sensor loss does not integrate a made-up current.
  float consumed=e.consumedAh();
  e.update(500100,NAN,NAN,false);e.update(500200,30.0f,12.0f,true);
  assert(std::fabs(e.consumedAh()-consumed)<0.00001f);
  LoadProtection p;
  // No trip from spikes shorter than 700 ms or stopped motor.
  p.evaluate(100,true,512,12,true,10,700,0,1500);
  p.evaluate(500,true,512,12,true,10,700,0,1500);
  assert(p.trip()==LoadProtection::Trip::None);
  p.evaluate(801,true,512,12,true,10,700,0,1500);
  assert(p.trip()==LoadProtection::Trip::HighCurrent);
  p.evaluate(900,false,0,0,true,10,700,0,1500);
  assert(p.trip()==LoadProtection::Trip::HighCurrent); // latch
  p.resetAfterRelease();
  assert(p.trip()==LoadProtection::Trip::None);
  p.evaluate(1200,true,500,0.1f,true,0,700,0.5f,1500);
  p.evaluate(2600,true,500,0.1f,true,0,700,0.5f,1500);
  assert(p.trip()==LoadProtection::Trip::None);
  p.evaluate(2701,true,500,0.1f,true,0,700,0.5f,1500);
  assert(p.trip()==LoadProtection::Trip::NoLoad);
  p.resetAfterRelease();
  p.evaluate(3000,true,100,40,true,10,700,0,1500); // ignore tiny startup duty
  p.evaluate(4000,true,100,40,true,10,700,0,1500);
  assert(p.trip()==LoadProtection::Trip::None);
  p.evaluate(4200,true,512,NAN,false,10,700,0,1500); // missing INA: degraded mode
  assert(p.trip()==LoadProtection::Trip::None);
  std::cout << "energy and load protection tests passed\n";
}
