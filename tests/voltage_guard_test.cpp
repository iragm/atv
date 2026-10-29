#include "voltage_guard.h"
#include <assert.h>
#include <stdio.h>
int main() {
  quad::VoltageGuard g;
  g.step(0,true,12.9f,13.0f); assert(!g.inhibited());
  g.step(1,true,13.01f,13.0f); assert(g.inhibited());
  g.step(5000,false,NAN,13.0f); assert(g.inhibited());
  g.step(6000,true,12.79f,13.0f); assert(g.inhibited());
  g.step(10999,true,12.79f,13.0f); assert(g.inhibited());
  g.step(11000,true,12.79f,13.0f); assert(!g.inhibited());
  g.step(12000,true,14.0f,13.0f); assert(g.inhibited());
  g.step(13000,true,12.75f,13.0f); assert(g.inhibited());
  g.step(14000,true,12.95f,13.0f); assert(g.inhibited());
  g.step(15000,true,12.75f,13.0f); assert(g.inhibited());
  g.step(20000,true,12.75f,13.0f); assert(!g.inhibited());
  puts("voltage guard tests passed");
}
