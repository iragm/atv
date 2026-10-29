# ATV controller

[![Firmware CI](https://github.com/iragm/atv/actions/workflows/firmware.yml/badge.svg)](https://github.com/iragm/atv/actions/workflows/firmware.yml) · [GPL-3.0](LICENSE)

ESP8266 D1 mini controls one motor through an **HW-039 / IBT-2**. Phone dashboard, configurable acceleration, current protection, temperature probes, battery gauge and OTA.

## Wiring

![Wiring diagram: complete color-coded power, motor, D1 mini, switch, INA226, lights and sensors](docs/diagrams/wiring-overview.svg)

[Open full-size diagram](docs/diagrams/wiring-overview.svg)

### D1 mini pins

| Pin | Connection |
|---|---|
| **5V** | Buck **OUT+**, adjusted to **5.00V** |
| **GND** | Buck OUT−, HW-039 GND, sensor GND and battery − (common ground) |
| **3V3** | HW-039 **VCC**, INA226 VCC, OLED VCC, both DS18B20 VCC; resistor pullups |
| **D0** | Throttle button to GND; external **10kΩ pullup to 3V3** |
| **D1 / D2** | I²C **SCL / SDA** to both INA226 and OLED |
| **D4** | Both DS18B20 DATA wires; **4.7kΩ pullup to 3V3** |
| **D5 / D6** | HW-039 **RPWM / LPWM**, each with **10kΩ pulldown** to GND |
| **D8** | HW-039 **R_EN and L_EN tied together**, with **10kΩ pulldown** to GND |
| **A0** | Direction switch resistor ladder (below) |
| D7 | Spare; no charger-jack switch contact |
| D3, RX, TX | Leave unused (boot and USB serial) |

**HW-039:** RPWM and LPWM control the *two halves of the same motor bridge*. Both are needed for forward/reverse. R_EN/L_EN are driver enables; D8 keeps them software-controlled rather than permanently ON. Leave R_IS/L_IS unconnected. The purchased board has an **SN74HC244**. Use **3.3V at its VCC header only after confirming that header powers the buffer**. Keep its onboard electrolytic; no extra bulk capacitor in the first build.

### Power, switch and lights

- **Motor:** Battery + → motor fuse → DPDT pole 1 COM → **both F and R throws joined** → external 50A/75mV shunt → HW-039 **B+**. Battery − → HW-039 **B−**. Motor → **M+ / M−**. Use suitably DC-rated wiring, fuse and switch.
- **Buck:** From the *switched* +12V (before the motor shunt) through its own fuse to buck VIN+. Buck VIN− → battery −; OUT+ → D1 mini **5V**, OUT− → GND.
- **Lights:** From the same switched +12V through their own fuse; return to battery −. **Always on in F or R, off in OFF.** Only connect directly if rated for 12V; original 6V lights need an appropriately rated regulator.
- **Charging:** 2-terminal smart AGM charger jack + → separate fuse → battery +, jack − → battery −. This works with the direction switch OFF. There is no electrical plug-in detector.

**Direction switch, pole 2 (isolated from the 12V pole):** COM → GND; F → **1kΩ** → A0; R → **10kΩ** → A0; A0 → **10kΩ** → 3V3 and separately → **10kΩ** → GND. Firmware accepts F/O/R as distinct ADC ranges. Check actual readings before running the motor.

Why not two GPIOs? The D1 mini has only one comfortably free GPIO (D7). D3/GPIO0 and D8/GPIO15 are boot-strapping pins; RX/TX are used by USB serial. **A0 preserves an unambiguous OFF reading without compromising startup.** Move to a board with more GPIOs if a future analog sensor needs A0.

### INA226 and the external shunt

| INA226 pin | Connection |
|---|---|
| **IN+** | Shunt **K+** Kelvin sense screw, on the switched supply side |
| **IN−** | Shunt **K−** Kelvin sense screw, on the HW-039 side |
| **VBUS** | HW-039 side of shunt **if breakout exposes VBUS and does not already tie it internally** |
| **VCC / GND** | D1 mini **3V3 / common GND** |
| **SCL / SDA** | D1 / D2 |

**No voltage divider is needed for the INA226.** It measures a 0–36V bus directly. The **50A/75mV shunt is 1.5mΩ**, as configured in firmware. The motor current passes through the *large shunt terminals*, **never through the INA226 board**. Only the two small Kelvin wires go to IN+/IN−. If your breakout has an onboard shunt (e.g. `R100`), it must not remain electrically in parallel with the external shunt. Verify its actual VBUS routing and I²C pullups to **3.3V** before applying power.

Motor-branch current is measured; charger current and accessories are not. When switched OFF, the INA226 and display are unpowered.

## Parts

| Part | Qty | Search |
|---|---:|---|
| [D1 mini ESP8266](https://www.google.com/search?q=D1+mini+ESP8266) | 1 | [Find](https://www.google.com/search?q=Wemos+D1+mini) |
| [HW-039 IBT-2 BTS7960 SN74HC244](https://www.google.com/search?q=HW039+IBT2+BTS7960) | 1 | [Find](https://www.google.com/search?q=BTS7960+HW-039+motor+driver) |
| [INA226 breakout + external 50A/75mV Kelvin shunt](https://www.google.com/search?q=INA226+external+shunt+50A+75mV) | 1 each | [Find](https://www.google.com/search?q=INA226+breakout+external+current+shunt) |
| [LM2596 buck 12V to 5V](https://www.google.com/search?q=LM2596+buck+converter) | 1 | [Find](https://www.google.com/search?q=LM2596+module) |
| [DS18B20 probes](https://www.google.com/search?q=DS18B20+temperature+probe) | 2 optional | [Find](https://www.google.com/search?q=waterproof+DS18B20) |
| [SSD1306 I²C OLED](https://www.google.com/search?q=SSD1306+128x64+i2c+oled) | 1 optional | [Find](https://www.google.com/search?q=SSD1306+0.96) |
| [12V AGM battery and smart charger](https://www.google.com/search?q=12V+AGM+battery+smart+charger) | 1 each | [Find](https://www.google.com/search?q=AGM+smart+charger+12V) |
| [DC-rated center-off DPDT, throttle, 2-wire jack](https://www.google.com/search?q=DC+DPDT+on+off+on+switch+barrel+jack) | 1 each | [Find](https://www.google.com/search?q=DPDT+center+off+DC+switch) |
| 10kΩ × **7**, 1kΩ × 1, 4.7kΩ × 1; branch fuses and wire | as needed | [Find](https://www.google.com/search?q=resistor+kit+inline+fuse+holder) |

I²C modules often have their own pullups. If neither does, add 4.7kΩ from each of SDA and SCL to 3V3. Don't tie I²C to 5V.

## Flash and use

1. Set the buck to **5.00V** before connecting the D1 mini. Confirm polarity and the HW-039 VCC pin with power **off**.
2. With motor disconnected, run `pio run -e d1_mini -t upload` and `pio device monitor -b 115200`.
3. Join `QUAD-XXXXXX`, password `quadsetup26`; open [192.168.4.1](http://192.168.4.1). Configure ramp, limits, sensors, Wi-Fi and battery voltages.
4. Test the switch, INA226 shunt polarity/current, driver logic and motor on a current-limited bench supply.

[Latest firmware (.bin)](https://github.com/iragm/atv/releases/download/firmware-latest/quad-controller-d1-mini.bin) · OTA from web UI with 2.4GHz internet credentials, or upload a `.bin` locally.

**Defaults:** 2s ramp, 6V forward / 4V reverse equivalent, 2s pedal release hold, 75°C motor / 70°C gearbox derating, 13.2V full / 11.8V empty battery gauge, charger-voltage lockout **above 13.9V**. Current cutoffs are off until calibrated. Missing OLED or sensors don't stop the controller, but their measurements/protections are unavailable. The 2-wire charging jack cannot detect an unpowered/low-voltage plugged-in charger.

## License

[GPL-3.0](LICENSE).