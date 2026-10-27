// Deterministic model-based stress testing of startup, direction changes,
// emergency inhibits and output cap. No Arduino libraries required.
#include "control.h"
#include "energy.h"
#include "settings.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

static uint32_t rng = 0x5A17A55Du;
static uint32_t next() { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; }

int main() {
  quad::Controller ctrl;
  quad::Input in;
  in.physicalDirection=quad::Direction::Forward;
  in.physicalPressed=false;
  ctrl.step(0,in);
  for (uint32_t t=10, n=0;n<100000;++n,t+=10) {
    auto r=next();
    in.physicalPressed = (r&1)!=0;
    in.wifiPressed=(r&2)!=0;
    in.charger=(r&4)!=0;
    in.physicalDirection=(r&8)?quad::Direction::Invalid:((r&16)?quad::Direction::Reverse:quad::Direction::Forward);
    in.wifiDirection=(r&32)?quad::Direction::Reverse:quad::Direction::Forward;
    in.emergencyCurrent=(r&64)!=0;
    in.overcurrent=(r&128)!=0;
    in.noLoad=(r&256)!=0;
    in.thermalLimit=(r&512)!=0;
    in.supplyVolts=9.0f+static_cast<float>((r>>12)%40)/10.0f;
    in.currentLimitVolts=static_cast<float>((r>>20)%120)/10.0f;
    const auto out=ctrl.step(t,in);
    assert(out.duty<=1023);
    if(in.charger || in.physicalDirection==quad::Direction::Invalid || in.emergencyCurrent || in.overcurrent || in.noLoad) {
      assert(!out.enabled && out.duty==0);
    }
    if(out.enabled) {
      assert(out.direction==quad::Direction::Forward || out.direction==quad::Direction::Reverse);
      const float maxV=out.direction==quad::Direction::Forward?6.0f:4.0f;
      float cap=maxV;
      if(in.thermalLimit&&cap>5)cap=5;
      if(in.currentLimitVolts<cap)cap=in.currentLimitVolts;
      assert(static_cast<float>(out.duty)<=std::floor(1023.0f*cap/in.supplyVolts)+1.0f);
    }
  }
  // Reject invalid protection ordering, preserve settings integrity.
  quad::Settings s;
  s.derateTripA=12;s.stallTripA=10;s.emergencyTripA=20;
  s.checksum=quad::settingsChecksum(s);
  assert(!quad::validSettings(s));
  s.derateTripA=6;s.checksum=quad::settingsChecksum(s);
  assert(quad::validSettings(s));
  // Stall timer must be reset by missing sensor or off-duty samples.
  quad::LoadProtection p;
  quad::LoadProtection::Settings thresholds;
  thresholds.stallA=9;thresholds.stallMs=200;
  p.evaluate(100,true,512,10,true,thresholds);
  p.evaluate(200,true,512,NAN,false,thresholds);
  p.evaluate(300,true,512,10,true,thresholds);
  p.evaluate(450,true,512,10,true,thresholds);
  assert(p.trip()==quad::LoadProtection::Trip::None);
  p.evaluate(501,true,512,10,true,thresholds);
  assert(p.trip()==quad::LoadProtection::Trip::HighCurrent);
  std::cout<<"adversarial property tests passed\n";
}
