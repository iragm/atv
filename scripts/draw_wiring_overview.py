#!/usr/bin/env python3
"""Produce the editable SVG overview embedded in README.md (not native Fritzing)."""
from pathlib import Path
import svgwrite
root = Path(__file__).resolve().parents[1]
output = root / 'docs/diagrams/wiring-overview.svg'
dwg = svgwrite.Drawing(str(output), size=('1440px', '850px'), viewBox='0 0 1440 850')
dwg.add(dwg.rect(insert=(0,0),size=(1440,850),fill='#f8fafc'))
colors = dict(red='#ce3939',blue='#1766b3',green='#07896f',violet='#7941a5',amber='#b67408',ink='#1e293b',gray='#64748b')
def line(a,b,color='ink',width=3,dash=None):
    attrs=dict(stroke=colors.get(color,color),stroke_width=width,fill='none',stroke_linecap='round',stroke_linejoin='round')
    if dash: attrs['stroke_dasharray']=dash
    dwg.add(dwg.polyline([a,b],**attrs))
def bend(points,color,width=3,dash=None):
    attrs=dict(stroke=colors.get(color,color),stroke_width=width,fill='none',stroke_linecap='round',stroke_linejoin='round')
    if dash:attrs['stroke_dasharray']=dash
    dwg.add(dwg.polyline(points,**attrs))
def text(x,y,s,size=17,color='ink',weight='normal'):
    dwg.add(dwg.text(s,insert=(x,y),fill=colors.get(color,color),font_family='Inter, Arial, sans-serif',font_size=size,font_weight=weight))
def box(x,y,w,h,title,lines,bar='blue'):
    dwg.add(dwg.rect(insert=(x,y),size=(w,h),rx=12,fill='#ffffff',stroke='#cbd5e1',stroke_width=1.5))
    dwg.add(dwg.rect(insert=(x,y),size=(w,7),rx=4,fill=colors.get(bar,bar)))
    text(x+16,y+35,title,20,'ink','bold')
    for i,s in enumerate(lines):text(x+16,y+62+i*22,s,14,'gray')
text(38,47,'QUAD CONTROLLER',29,'ink','bold')
text(375,46,'REV C  |  ESP8266 direct control  |  3 ICs removed',16,'gray')
text(40,84,'POWER AND LOAD PATH',16,'blue','bold')
box(40,105,206,126,'12V AGM battery',['+ / - terminals','charger branch bypasses OFF'],'red')
box(292,105,221,126,'Fuse + DPDT',['motor fuse provisional 10A','power pole: F/O/R -> SW_12V'],'red')
box(569,105,204,126,'50A / 75mV',['external Kelvin shunt','1.5 mOhm; high-side B+'],'red')
box(835,105,257,126,'IBT-2 / BTS7960',['B+ / B- input power','M+ / M- to motor'],'red')
box(1161,105,235,126,'Original DC motor',['Originally rated 6V','12V PWM pulses: bench test'],'red')
for a,b in [((246,151),(292,151)),((513,151),(569,151)),((773,151),(835,151)),((1092,151),(1161,151))]:line(a,b,'red',5)
text(40,263,'LOGIC AND INPUT SIGNAL PATH',16,'blue','bold')
box(40,302,228,118,'LM2596 buck',['from switched fused 12V','5.0V logic rail'],'amber')
box(356,302,377,249,'Wemos D1 mini / ESP8266',[
    'D0 throttle  |  D7 charger jack',
    'A0 forward/reverse ladder',
    'D5 -> RPWM   |   D6 -> LPWM',
    'D8 -> R_EN + L_EN',
    'D1/D2 -> I2C   |   D4 -> 1-Wire',
    'D3 free; RX / TX for serial',
    'software only: charger + throttle inhibit'
],'blue')
box(826,301,274,113,'IBT-2 control header',['direct 3.3V GPIO signals','validate onboard 74HC244'],'violet')
box(1162,304,234,109,'Barrel charger jack',['12V smart AGM charger','D7 plug-detect sleeve NC'],'amber')
line((268,364),(356,364),'amber',4)
bend([(733,360),(776,360),(776,351),(826,351)],'violet',4)
bend([(1162,363),(1125,363),(1125,624),(776,624),(776,520),(733,520)],'green',3)
text(859,279,'EN + PWM',13,'violet','bold')
box(41,459,226,122,'Throttle button',['normally open, to GND','D0 = LOW when pressed'],'green')
box(41,626,226,127,'Direction switch',['isolated DPDT pole','F = 1k / R = 10k to A0'],'green')
box(830,463,275,131,'INA226 + OLED',['shared SDA/SCL on D2/D1','INA226 shunt sense wires','OLED: 3.3V logic'],'blue')
box(1162,465,233,130,'Two DS18B20s',['shared D4 (GPIO2)','4.7k pullup to 3.3V','T1/T2 pending assignment'],'blue')
bend([(268,514),(321,514),(321,449),(356,449)],'green',3)
bend([(267,684),(303,684),(303,489),(356,489)],'green',3)
bend([(733,484),(780,484),(780,532),(830,532)],'blue',3)
bend([(733,534),(750,534),(750,650),(1268,650),(1268,595)],'blue',3)
text(41,797,'SIMPLIFIED TOPOLOGY. Detailed connections, returns, fuses and ADC resistor values: see wiring-netlist.md.',15,'ink','bold')
text(41,823,'This is an SVG wiring overview, not a native .fzz breadboard diagram or validated electronic schematic.',13,'gray')
dwg.save()
print(output)
