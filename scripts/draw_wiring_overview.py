#!/usr/bin/env python3
"""Unified, pin-complete HW-039 ATV wiring diagram (illustrative, not .fzz).

Wires meet electrically only at junction dots or component pins. Colors identify nets.
Update this diagram alongside firmware pin assignments in include/config.h.
"""
from pathlib import Path
import svgwrite

OUT=Path(__file__).resolve().parents[1]/'docs/diagrams/wiring-overview.svg'
OUT.parent.mkdir(parents=True,exist_ok=True)
W,H=3000,2200
s=svgwrite.Drawing(str(OUT),size=(f'{W}px',f'{H}px'),viewBox=f'0 0 {W} {H}')
WIRES=[]
PINS=[]
C=dict(bg='#f7fafc',title='#14283a',ink='#20384a',muted='#587187',panel='#ffffff',edge='#b6c5d0',plus='#e55353',gnd='#3878b9',sw='#ea3b3b',v5='#d5a02a',v3='#e2a63e',pwm='#a75ac8',en='#8054ba',motor1='#dc7332',motor2='#8669d9',i2c='#1889b0',onewire='#16a080',sense='#07989b',adc='#ce7951',throttle='#7c8999',low='#e9f2fa')

def L(text,x,y,size=19,color='ink',bold=False,anchor='start'):
 s.add(s.text(text,insert=(x,y),fill=C.get(color,color),font_size=size,font_family='DejaVu Sans,Arial,sans-serif',font_weight='bold' if bold else 'normal',text_anchor=anchor))
def R(x,y,w,h,fill='panel',r=15,stroke='edge',sw=2):
 s.add(s.rect(insert=(x,y),size=(w,h),rx=r,fill=C.get(fill,fill),stroke=C.get(stroke,stroke),stroke_width=sw))
def wire(points,color='gnd',width=6,dash=None):
 WIRES.append((tuple(points),color))
 p=s.polyline(points,stroke=C.get(color,color),fill='none',stroke_width=width,stroke_linecap='round',stroke_linejoin='round')
 if dash:p['stroke-dasharray']=dash
 s.add(p)
def pad(x,y,color):s.add(s.circle(center=(x,y),r=7,stroke=C[color],stroke_width=3,fill='white'))
def dot(x,y,color):s.add(s.circle(center=(x,y),r=8,stroke='white',stroke_width=2,fill=C[color]))
def block(x,y,w,h,title,sub,accent='gnd'):
 R(x,y,w,h)
 s.add(s.rect(insert=(x+2,y+2),size=(w-4,11),rx=5,fill=C[accent]))
 L(title,x+18,y+43,26,'ink',True)
 L(sub,x+18,y+71,16,'muted')
def pin(x,y,name,color='gnd',side='left'):
 PINS.append((x,y,name,color))
 pad(x,y,color)
 L(name,x+18 if side=='left' else x-18,y+6,18,'ink',True,'start' if side=='left' else 'end')
def resistor(x,y,name,orient='h',color='adc'):
 if orient=='h':
  R(x,y-14,70,28,'#fff8ec',4,color,2)
  L(name,x+35,y+6,16,'ink',True,'middle')
 else:
  R(x-21,y,42,54,'#fff8ec',4,color,2)
  L(name,x,y+32,15,'ink',True,'middle')

# Background and headline
s.add(s.rect(insert=(0,0),size=(W,H),fill=C['bg']))
s.add(s.rect(insert=(0,0),size=(W,122),fill=C['title']))
L('ATV  |  COMPLETE WIRING',54,63,40,'#ffffff',True)
L('D1 mini  +  HW-039 (SN74HC244)  +  INA226 external shunt',54,98,20,'#cadeef')
L('One shared circuit • all active pins • colors = wire functions',2930,65,18,'#cadeef',False,'end')

# Net distribution: visible common ground and 3.3V rails
wire([(400,500),(400,2020),(2800,2020)],'gnd',10) # common battery negative
wire([(1530,1960),(2800,1960)],'v3',9) # distributed 3.3 V; fed by D1
L('COMMON BATTERY − / LOGIC GND',560,2047,21,'gnd',True)
L('D1 MINI 3V3 RAIL',1850,1948,18,'#a06e14',True)

# 12V high-current path and parallel switched branch: drawn behind modules.
wire([(400,370),(535,370)],'plus',9)
wire([(735,370),(758,370),(758,390),(780,390)],'plus',9)
wire([(1165,353),(1218,353),(1218,405),(1380,405)],'sw',9)
wire([(1165,409),(1218,409)],'sw',9)
dot(1218,409,'sw')
wire([(1620,405),(1800,405)],'plus',9)
# HW driver high-current return to battery GND
wire([(1800,490),(1746,490),(1746,2020)],'gnd',9)
# Motor outputs
wire([(2270,560),(2455,560),(2455,425),(2560,425)],'motor1',9)
wire([(2270,625),(2490,625),(2490,505),(2560,505)],'motor2',9)
# switched rail taps to lights and buck
wire([(1290,405),(1290,884),(945,884)],'sw',7)
wire([(1325,405),(1325,963),(1395,963)],'sw',7)
dot(1290,405,'sw');dot(1325,405,'sw')
# charger + independent of DPDT; fuse in series
wire([(453,370),(453,895),(505,895)],'plus',6)
wire([(665,895),(728,895),(728,964),(470,964)],'plus',6)
wire([(470,1030),(485,1030),(485,2020)],'gnd',6)
dot(453,370,'plus')
# lights + fused (12V rated only), return to common GND
wire([(945,884),(945,1110),(795,1110)],'sw',6)
wire([(550,1130),(550,2020)],'gnd',6)
# buck supply branch + fuse, ground return
wire([(1395,963),(1422,963)],'sw',6)
wire([(1572,963),(1590,963),(1590,1070),(1220,1070)],'sw',6)
wire([(935,1115),(875,1115),(875,2020)],'gnd',6)
# buck 5V to D1 mini 5V (not 3V3 pin)
wire([(1220,1170),(1250,1170),(1250,1240),(1120,1240),(1120,1360),(1180,1360)],'v5',6)
wire([(935,1215),(900,1215),(900,1410),(1180,1410)],'gnd',5)
# separate ground at 5V supply physically same GND return
# D1 mini logic supply distribution
wire([(1730,1380),(1730,1960)],'v3',7)
wire([(1730,1420),(1760,1420),(1760,2020)],'gnd',6)
dot(1730,1960,'v3')
# bridge buffer logic connections
# Driver logic supply wired only at its VCC/GND pins on the RIGHT; never to L_EN.
dot(1768,1960,'v3')
# D1 to IBT-2 control signals
wire([(1730,1500),(1760,1500),(1760,590),(1800,590)],'pwm',5)
wire([(1730,1540),(1778,1540),(1778,635),(1800,635)],'pwm',5)
wire([(1730,1580),(1795,1580),(1795,675),(1800,675)],'en',5)
wire([(1795,675),(1825,675),(1825,715),(1800,715)],'en',5)
dot(1795,675,'en')
# Three passive 10k control pulldowns (boot/noise defaults). They do NOT replace D8 enable control.
for px,py,from_x,from_y,net in [(1810,1500,1760,1500,'pwm'),
                                   (1845,1540,1778,1540,'pwm'),
                                   (1880,1580,1795,1580,'en')]:
 wire([(from_x,from_y),(px,py),(px,1778)],net,4)
 resistor(px,1778,'10k','v',net)
 wire([(px,1832),(px,2020)],'gnd',4)
 dot(px,from_y,net)
# Motor-driver enable pad is low at ESP8266 boot; both R_EN/L_EN tied to D8.
# INA shunt sense and bus voltage. Each input directly at shunt Kelvin screw;
# dashed for low current *sense* wires, separate from motor power path.
wire([(1455,500),(1455,1000),(1875,1000),(1875,1140),(1970,1140)],'sense',4,'12,9')
wire([(1555,500),(1555,1032),(1900,1032),(1900,1180),(1970,1180)],'sense',4,'12,9')
wire([(1690,405),(1690,962),(1920,962),(1920,1220),(1970,1220)],'sense',4,'12,9')
dot(1690,405,'plus')
# Sensor power rails and signals
wire([(2425,1410),(2495,1410),(2495,1960)],'v3',5)
wire([(2425,1450),(2510,1450),(2510,2020)],'gnd',5)
dot(2495,1960,'v3')
wire([(1730,1660),(1850,1660),(1850,1300),(1970,1300)],'i2c',5) # SDA
wire([(1730,1620),(1830,1620),(1830,1340),(1970,1340)],'i2c',5) # SCL
wire([(1850,1300),(2440,1300),(2440,1080),(2540,1080)],'i2c',5)
wire([(1830,1340),(2470,1340),(2470,1120),(2540,1120)],'i2c',5)
dot(1850,1300,'i2c');dot(1830,1340,'i2c')
wire([(2540,1190),(2500,1190),(2500,1960)],'v3',5)
wire([(2540,1230),(2520,1230),(2520,2020)],'gnd',5)
dot(2500,1960,'v3')
wire([(1730,1700),(2460,1700),(2460,1450),(2540,1450)],'onewire',5)
wire([(2540,1520),(2530,1520),(2530,1960)],'v3',5)
wire([(2540,1570),(2545,1570),(2545,2020)],'gnd',5)
dot(2530,1960,'v3')
# 4.7k data pullup branches from 1-Wire into 3.3V
wire([(2420,1700),(2420,1760)],'onewire',4)
resistor(2420,1760,'4.7k',orient='v',color='onewire')
wire([(2420,1814),(2420,1960)],'v3',4)
dot(2420,1700,'onewire');dot(2420,1960,'v3')
# throttle switch to GPIO16 and passive pullup (10k to 3v3)
wire([(860,1540),(1160,1540),(1160,1600),(1180,1600)],'throttle',5)
wire([(580,1610),(580,2020)],'gnd',5)
wire([(1015,1540),(1015,1680)],'throttle',4)
resistor(1015,1680,'10k',orient='v',color='throttle')
wire([(1015,1734),(1015,1960),(1530,1960)],'v3',5)
dot(1015,1540,'throttle');dot(1015,1960,'v3')
# analog direction: pole2 COM -> common GND; two contact-specific series resistors
wire([(780,635),(750,635),(750,2020)],'gnd',5)
wire([(1165,645),(1220,645)],'adc',4)
resistor(1220,645,'1k','h','adc')
wire([(1290,645),(1370,645),(1370,1520),(1180,1520)],'adc',4)
wire([(1165,700),(1240,700)],'adc',4)
resistor(1240,700,'10k','h','adc')
wire([(1310,700),(1370,700)],'adc',4)
dot(1370,700,'adc')
# A0 midpoint bias network. Only connected on external D1 mini A0 (3.3V max BOARD input).
wire([(1180,1520),(1025,1520)],'adc',4)
wire([(1125,1520),(1125,1730)],'adc',4)
resistor(1125,1730,'10k','v','adc')
wire([(1125,1784),(1125,1960)],'v3',4)
wire([(1060,1520),(1060,1800)],'adc',4)
resistor(1060,1800,'10k','v','adc')
wire([(1060,1854),(1060,2020)],'gnd',4)
dot(1125,1520,'adc');dot(1060,1520,'adc')
dot(1125,1960,'v3');dot(1060,2020,'gnd')

# Component bodies on top of the wires
block(90,265,310,300,'12V AGM BATTERY','Lead-acid • fused branches','plus')
pin(400,370,'+','plus','right');pin(400,500,'−','gnd','right')
L('Charger is connected even',110,455,18,'muted')
L('while direction is OFF',110,481,18,'muted')
block(535,313,200,110,'MOTOR FUSE','Size for cable + load','plus')
pin(535,370,'IN','plus');pin(735,370,'OUT','plus','right')
block(780,270,385,505,'3-POSITION DPDT','F / OFF / R • two isolated poles','sw')
L('POLE 1   SWITCHED +12V',797,353,19,'sw',True)
pin(780,390,'COM + IN','plus');pin(1165,353,'F + OUT','sw','right');pin(1165,409,'R + OUT','sw','right')
L('F and R OUTPUTS JOIN',985,460,15,'muted',True)
L('OFF cuts buck, lights, motor',798,497,17,'muted')
L('POLE 2   DIRECTION (3 STATES)',798,583,18,'adc',True)
pin(780,635,'COM → GND','gnd');pin(1165,645,'F → 1k → A0','adc','right');pin(1165,700,'R → 10k → A0','adc','right')
L('A0 also: 10k → 3V3, 10k → GND',798,745,16,'muted')
block(1380,298,240,222,'50A / 75mV','1.5mΩ • 4-terminal shunt','sense')
pin(1380,405,'LOAD IN','sw');pin(1620,405,'LOAD OUT','plus','right')
pin(1455,500,'K+','sense');pin(1555,500,'K−','sense','right')
L('Large wires: motor current',1397,455,15,'muted')
L('Small wires: Kelvin sense',1397,479,15,'muted')
block(1800,270,470,620,'HW-039 / IBT-2','SN74HC244 onboard buffer • 3V3 VCC','en')
for y,name,color in [(405,'B+  fused switched motor +','plus'),(490,'B−  battery GND','gnd'),(590,'RPWM  D5','pwm'),(635,'LPWM  D6','pwm'),(675,'R_EN  D8','en'),(715,'L_EN  D8','en'),(710,'VCC  3V3','v3'),(755,'GND  common','gnd')]:
 # VCC at y710 would clash L_EN at715; move VCC to 770, GND 815 below
 if y in (710,755) and name.startswith(('VCC','GND')):continue
 pin(1800,y,name,color)
# show logic pins on right side, easier separate from left signal pins
pin(2270,770,'VCC = 3V3','v3','right');pin(2270,815,'GND','gnd','right')
# relocate already routed 3v3/gnd to right pins with short wire (do not leave left ghosts)
# (No hidden board-internal cross-wires; VCC/GND are supplied on the right pins.)
L('R_IS / L_IS: NC (unused)',1830,835,17,'muted')
L('Onboard bulk capacitor: keep',1830,865,16,'muted')
L('BTS7960: two halves, ONE motor',1830,545,16,'muted')
block(2560,325,320,330,'DC MOTOR','Original 6V • PWM from 12V','motor1')
pin(2560,425,'M+','motor1');pin(2560,505,'M−','motor2')
L('Verify motor heating',2578,582,17,'muted')
L('and stall current',2578,608,17,'muted')
block(100,900,370,210,'2-WIRE CHARGER JACK','12V AGM smart charger','plus')
pin(470,964,'JACK +','plus','right');pin(470,1030,'JACK −','gnd','right')
L('Jack + through fuse to BAT +',120,1066,16,'muted')
block(505,840,160,107,'CHARGE FUSE','Near battery +','plus')
pin(505,895,'+','plus');pin(665,895,'+','plus','right')
block(550,1050,245,170,'LIGHTS','Constant ON in F or R','sw')
pin(795,1110,'+ via fuse','sw','right');pin(550,1130,'− GND','gnd')
L('12V-rated only!',571,1184,17,'#a64421',True)
# show light fuse and branch visually
resistor(1010,884,'FUSE','h','sw')
block(1422,908,150,105,'BUCK FUSE','Switched 12V','sw')
pin(1422,963,'IN','sw');pin(1572,963,'OUT','sw','right')
block(935,985,285,250,'LM2596 BUCK','Adjust OUT to 5.00V','v5')
pin(1220,1070,'VIN+','sw','right');pin(935,1115,'VIN−','gnd')
pin(1220,1170,'OUT+ 5V','v5','right');pin(935,1215,'OUT−','gnd')
block(1180,1255,550,520,'ESP8266 D1 MINI','USB serial kept free • 3.3V IO','v3')
for y,name,color in [(1360,'5V (from buck)','v5'),(1410,'GND (from buck)','gnd'),(1520,'A0: resistor ladder','adc'),(1600,'D0: throttle','throttle')]:
 pin(1180,y,name,color)
for y,name,color in [(1380,'3V3: logic rail','v3'),(1420,'GND: common','gnd'),(1500,'D5: RPWM','pwm'),(1540,'D6: LPWM','pwm'),(1580,'D8: EN ×2','en'),(1620,'D1: SCL','i2c'),(1660,'D2: SDA','i2c'),(1700,'D4: DS18B20','onewire')]:
 pin(1730,y,name,color,'right')
L('D7: spare • D3: boot pin',1198,1690,16,'muted')
L('RX/TX: serial • D4/D8 boot straps',1198,1717,16,'muted')
block(520,1470,340,170,'THROTTLE BUTTON','Normally open • active LOW','throttle')
pin(860,1540,'D0','throttle','right');pin(580,1610,'GND','gnd')
L('10k pull-up to 3V3',548,1595,17,'muted')
block(1970,1070,455,450,'INA226 CURRENT','External shunt monitor','sense')
for y,name,color in [(1140,'IN+ ← K+ before shunt','sense'),(1180,'IN− ← K− after shunt','sense'),(1220,'VBUS ← after shunt*','sense'),(1300,'SDA ← D2','i2c'),(1340,'SCL ← D1','i2c'),(1410,'VCC ← 3V3','v3'),(1450,'GND','gnd')]:
 pin(1970 if y<=1340 else 2425,y,name,color, 'left' if y<=1340 else 'right')
L('* Check breakout VBUS trace',1990,1490,16,'muted')
L('No divider: INA226 VBUS ≤36V',1990,1512,16,'muted')
block(2540,990,360,270,'OLED I²C','SSD1306 128×64 optional','i2c')
for y,name,col in [(1080,'SDA ← D2','i2c'),(1120,'SCL ← D1','i2c'),(1190,'VCC ← 3V3','v3'),(1230,'GND','gnd')]:pin(2540,y,name,col)
block(2540,1345,360,295,'2× DS18B20','OneWire in parallel; optional','onewire')
for y,name,col in [(1450,'DATA ← D4','onewire'),(1520,'VCC ×2 ← 3V3','v3'),(1570,'GND ×2','gnd')]:pin(2540,y,name,col)
L('4.7k pull-up DATA → 3V3',2558,1610,16,'muted')
# Short finish to driver power pins on right: branch to rails outside panel
wire([(2270,770),(2390,770),(2390,1960)],'v3',5)
wire([(2270,815),(2415,815),(2415,2020)],'gnd',5)
dot(2390,1960,'v3')
# Entire wiring note / legend
L('POWER',100,2100,20,'muted',True)
for x,color,description in [(215,'plus','12V +'),(430,'gnd','GND'),(640,'v5','5V'),(830,'v3','3.3V'),(1040,'motor1','M+'),(1220,'motor2','M−'),(1410,'pwm','PWM'),(1620,'en','EN'),(1810,'i2c','I²C'),(1990,'onewire','1-Wire'),(2220,'sense','INA sense'),(2500,'adc','A0 / direction')]:
 s.add(s.line(start=(x,2094),end=(x+48,2094),stroke=C[color],stroke_width=8,stroke_linecap='round'))
 L(description,x+55,2102,18,'ink')
L('Disconnected / unused: HW-039 R_IS, L_IS; D1 mini D3, D7, RX, TX; OLED and DS18B20 optional.',95,2162,17,'muted')
def touches(x,y,points):
 for (x1,y1),(x2,y2) in zip(points,points[1:]):
  if (x1==x2==x and min(y1,y2)<=y<=max(y1,y2)) or (y1==y2==y and min(x1,x2)<=x<=max(x1,x2)):
   return True
 return False
missing=[name for x,y,name,color in PINS if not any(touches(x,y,points) for points,net in WIRES if net==color)]
if missing: raise RuntimeError('Unwired or wrong-color component pins: '+str(missing))
s.save()
print(OUT, OUT.stat().st_size, 'validated pins',len(PINS))