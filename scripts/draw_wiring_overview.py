#!/usr/bin/env python3
"""Create an exact pin-labeled, Fritzing-style SVG wiring illustration.

The source of truth for the project wiring is the README pin map.
The illustrated modules are stylized, not to physical scale or verified footprints.
"""
from pathlib import Path
from xml.sax.saxutils import escape
import svgwrite

ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT / 'docs/diagrams/wiring-overview.svg'
DEST.parent.mkdir(parents=True, exist_ok=True)
W, H = 1940, 1600
s = svgwrite.Drawing(str(DEST), size=(f'{W}px', f'{H}px'), viewBox=f'0 0 {W} {H}')
C = {'bg':'#f6f7f8','board':'#ffffff','outline':'#bfcbd7','text':'#202b39','muted':'#5a697b',
     'rail12':'#df4c47','gnd':'#3b6689','v33':'#e6a124','v5':'#d18b14',
     'pwm':'#b149a6','en':'#8554cd','motorA':'#f27a49','motorB':'#735bd1','onewire':'#269477','i2c':'#1b92bb',
     'signal':'#727e91','sense':'#16a3a4','soft':'#eaf0f3'}
s.add(s.rect(insert=(0,0),size=(W,H),fill=C['bg']))
layer = s
def add(el): layer.add(el)
def txt(x,y,t,size=22,color='text',bold=False,anchor='start'):
    add(s.text(t, insert=(x,y),fill=C.get(color,color),font_family='Arial, Helvetica, sans-serif',font_weight='700' if bold else '400',font_size=size,text_anchor=anchor))
def rect(x,y,w,h,fill='board',stroke='outline',rad=16,sw=2):
    add(s.rect(insert=(x,y),size=(w,h),rx=rad,fill=C.get(fill,fill),stroke=C.get(stroke,stroke),stroke_width=sw))
def path(points,color,width=6,dash=None):
    attrs={'fill':'none','stroke':C.get(color,color),'stroke_width':width,'stroke_linecap':'round','stroke_linejoin':'round'}
    if dash: attrs['stroke_dasharray']=dash
    add(s.polyline(points,**attrs))
def junction(x,y,color): add(s.circle(center=(x,y),r=8,fill=C[color],stroke='#fff',stroke_width=2))
def label_pin(x,y,title,side='left',color='signal'):
    # x = pad boundary, side = label inside box direction
    add(s.circle(center=(x,y),r=8,fill='#f9fbfb',stroke=C[color],stroke_width=3))
    txt(x+19 if side=='left' else x-19,y+6,title,19,'text',True,anchor='start' if side=='left' else 'end')
def chip(x,y,w,h,title,subtitle,accent='gnd'):
    rect(x,y,w,h)
    add(s.rect(insert=(x+2,y+2),size=(w-4,9),rx=4,fill=C[accent]))
    txt(x+24,y+44,title,29,'text',True)
    txt(x+24,y+75,subtitle,18,'muted')

def note(x,y,msg): txt(x,y,msg,18,'muted')

# Banner
add(s.rect(insert=(0,0),size=(W,121),fill='#19293b'))
txt(52,61,'ATV • PIN-TO-PIN WIRING',39,'#ffffff',True)
txt(53,93,'HW-039 / IBT-2  •  SN74HC244  •  ESP8266 D1 mini',20,'#c9e1ef')
txt(1890,61,'Rev G',25,'#a6e1ca',True,anchor='end')

# Section framing
rect(34,151,1871,793,fill='#edf2f6',stroke='#d2dce3',rad=22,sw=2)
txt(57,192,'1. CONTROL & SIGNALS',25,'text',True)

# Signal wires are drawn first, behind component bodies.
# LEFT: Buck 5 V, throttle active-low, charger detect, direction A0 ladder
path([(422,318),(551,318)],'v5')
path([(422,422),(551,422)],'signal')
path([(422,525),(551,525)],'signal')
path([(422,635),(551,635)],'signal')
# RIGHT: 3.3V and common ground + driver controls
for y,c in [(340,'v33'),(384,'gnd'),(433,'pwm'),(481,'pwm'),(529,'en')]:
    path([(936,y),(1213,y)],c)
path([(1091,529),(1091,574),(1213,574)],'en')
junction(1091,529,'en')
# OneWire and I2C connections to sensors
for y,c in [(749,'onewire'),(803,'i2c'),(855,'i2c')]:
    path([(936,y),(1213,y)],c)

# Left-side inputs
chip(68,269,354,98,'LM2596 buck','OUT+ = regulated 5.0 V',accent='v5')
label_pin(422,318,'',color='v5')
chip(68,375,354,93,'Throttle switch','NO contact to GND • D0 LOW = pressed',accent='signal')
label_pin(422,422,'',color='signal')
chip(68,475,354,93,'Charger jack sense','NC contact → GND when unplugged',accent='signal')
label_pin(422,525,'',color='signal')
chip(68,582,354,198,'F/O/R DPDT: pole 2','COM → GND  •  independent contacts',accent='signal')
label_pin(422,635,'',color='signal')
note(89,695,'F → 1kΩ → A0   /   R → 10kΩ → A0')
note(89,727,'A0 → 10kΩ → 3V3; A0 → 10kΩ → GND')
note(89,760,'SWITCH OFF → A0 in neither valid range')

# D1 mini stylized actual board
chip(551,215,385,689,'D1 mini ESP8266','USB programming and Wi-Fi',accent='gnd')
for yy,ln,col in [(318,'5V','v5'),(422,'D0  throttle','signal'),(525,'D7  charger','signal'),(635,'A0  F/O/R','signal')]:
    label_pin(551,yy,ln,'left',col)
for yy,ln,col in [(340,'3V3','v33'),(384,'GND','gnd'),(433,'D5 / RPWM','pwm'),(481,'D6 / LPWM','pwm'),(529,'D8 / EN','en'),
                  (749,'D4 / 1-Wire','onewire'),(803,'D1 / SCL','i2c'),(855,'D2 / SDA','i2c')]:
    label_pin(936,yy,ln,'right',col)

# Driver header and sensor harness
chip(1213,242,623,370,'HW-039 / IBT-2 control','SN74HC244 confirmed on purchased board',accent='en')
for y,t,col in [(340,'VCC  ←  3V3','v33'),(384,'GND  ←  common GND','gnd'),(433,'RPWM  ←  D5','pwm'),
                (481,'LPWM  ←  D6','pwm'),(529,'R_EN  ←  D8','en'),(574,'L_EN  ←  D8','en')]:
    label_pin(1213,y,t,'left',col)
rect(1490,507,321,89,fill='#fff9e8',stroke='#edcc8b',rad=10)
txt(1512,534,'3.3V BUFFER SUPPLY',17,'#80530f',True)
txt(1512,558,'Check VCC continuity',16,'#80530f')
txt(1512,580,'with power disconnected',16,'#80530f')
chip(1213,645,623,244,'Sensor bus (optional)','OLED  +  INA226  +  2 × DS18B20',accent='onewire')
for y,t,col in [(749,'D4 → DS18B20 DATA × 2  •  4.7kΩ → 3V3','onewire'),
                (803,'D1 → SCL: OLED + INA226','i2c'),
                (855,'D2 → SDA: OLED + INA226','i2c')]:
    label_pin(1213,y,t,'left',col)


# Power path section
layer = s.g(transform='translate(0,120)')
s.add(layer)
rect(34,866,1871,504,fill='#f2f5f7',stroke='#d2dce3',rad=22)
txt(57,909,'2. BATTERY & MOTOR POWER',25,'text',True)

# Rails: keep red battery positive, blue battery negative, motor leads separate.
for aa,bb in [((300,1094),(390,1094)),((553,1094),(616,1094)),((802,1094),(852,1094)),((1076,1094),(1152,1094))]:
    path([aa,bb],'rail12',8)
path([(300,1172),(334,1172),(334,1222),(1152,1222)],'gnd',7)
path([(1388,1094),(1578,1094)],'motorA',7)
path([(1388,1160),(1578,1160)],'motorB',7)
# Charger bypasses the DPDT.
path([(340,1094),(340,1301),(405,1301)],'rail12',6)
path([(350,1222),(350,1338),(405,1338)],'gnd',6)
junction(340,1094,'rail12')
junction(350,1222,'gnd')
# Switched 12V tap feeds 5V buck, separately fused.
path([(826,1094),(826,1254),(979,1254)],'rail12',5)
junction(826,1094,'rail12')
# Kelvin sense pair branches from the shunt and runs to the INA226.
path([(913,1149),(913,1288),(1240,1288)],'sense',4,'10,8')
path([(1033,1149),(1033,1322),(1240,1322)],'sense',4,'10,8')

def power_card(x,y,w,h,name,subtitle,accent='rail12'):
    rect(x,y,w,h)
    add(s.rect(insert=(x+2,y+2),size=(w-4,9),rx=4,fill=C[accent]))
    txt(x+18,y+34,name,23,'text',True)
    txt(x+18,y+62,subtitle,16,'muted')

power_card(70,949,230,261,'12V AGM','Battery + / −')
power_card(390,949,163,194,'Motor fuse','10A: verify rating')
power_card(616,949,186,194,'DPDT pole 1','F / R switched')
power_card(852,949,224,194,'50A / 75mV','1.5mΩ shunt','sense')
power_card(1152,949,236,307,'HW-039 power','same board above')
power_card(1578,949,267,257,'DC motor','original 6V motor')
for x,y,c in [(300,1094,'rail12'),(300,1172,'gnd'),(1152,1094,'rail12'),(1152,1222,'gnd'),(1388,1094,'motorA'),(1388,1160,'motorB'),(1578,1094,'motorA'),(1578,1160,'motorB')]:
    add(s.circle(center=(x,y),r=8,fill='#fff',stroke=C[c],stroke_width=3))
txt(1174,1101,'B+ supply',18,'text',True)
txt(1174,1230,'B− / BAT−',18,'text',True)
txt(1366,1081,'M+',17,'text',True,anchor='end')
txt(1366,1147,'M−',17,'text',True,anchor='end')
txt(1597,1080,'M+',18,'text',True)
txt(1597,1148,'M−',18,'text',True)

rect(405,1267,488,90,fill='#fff',stroke='#d4dade',rad=12)
txt(425,1293,'AGM smart charger jack',20,'text',True)
txt(425,1319,'Center + (fused) • Sleeve −',17,'muted')
txt(425,1344,'D7: verify switched NC sleeve contact',16,'muted')
rect(979,1214,425,72,fill='#fff5e4',stroke='#e9c688',rad=10)
txt(995,1240,'Switched 12V → fused buck VIN+',17,'#74521c',True)
txt(995,1267,'Buck OUT+ 5V → D1 mini 5V',17,'#74521c')
rect(1240,1268,602,90,fill='#e5f7f5',stroke='#9ad7d2',rad=12)
txt(1260,1292,'INA226 sense (separate small wires)',17,'#136f69',True)
txt(1260,1318,'IN+ before shunt • IN− after shunt',17,'#136f69')
txt(1260,1344,'VBUS after shunt, if module requires',16,'#136f69')

# Compact two-line color legend.
colors=[('rail12','Battery +12V'),('gnd','Battery GND'),('motorA','Motor M+'),('motorB','Motor M−'),('v33','Logic 3.3V'),('v5','Logic 5V'),
        ('pwm','PWM'),('en','Enable'),('i2c','I²C'),('onewire','1-Wire'),('signal','Switch'),('sense','Shunt sense')]
for i,(col,title) in enumerate(colors):
    row=i//6
    xx=64+(i%6)*309
    yy=1395+row*36
    add(s.line(start=(xx,yy),end=(xx+43,yy),stroke=C[col],stroke_width=8,stroke_linecap='round'))
    txt(xx+55,yy+7,title,19,'text')

s.save()
print('Created',DEST)
