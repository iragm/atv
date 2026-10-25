# Wiring netlist, revision C: three ICs removed

This is a **provisional textual netlist**, not a validated Fritzing schematic. It supersedes the old revision B netlist from the ZIP. There is NO 74AHCT02, 74AHCT125 or MCP23008, no discrete-transistor inhibit and no relay. ESP8266 controls the IBT-2 enable signal directly.

## Battery and power nets

- `BAT_POS`: AGM battery positive; `BAT_NEG`: battery negative / high-current common reference.
- `BAT_POS -> MOTOR_FUSE (existing 10A provisional) -> DPDT POWER COM`. Its forward and reverse throws are **jumpered** together -> `SW_12V`; center-off disconnects. Verify DPDT has isolated poles, mechanical break-before-make, and adequate **DC** current rating.
- `SW_12V -> EXTERNAL_SHUNT_IN -> 50A/75mV shunt -> EXTERNAL_SHUNT_OUT -> IBT2 B+`. `IBT2 B- -> BAT_NEG`. `IBT2 M+ / M- -> motor terminals only`.
- `SW_12V -> fused buck branch -> LM2596 VIN+`; buck `VIN- -> BAT_NEG`; set buck output to 5.0V and connect `LOGIC_5V -> D1 Mini 5V` and `IBT2 VCC` **only if its board requires 5V**; buck GND -> common logic ground. Keep high-current returns away from logic wiring.
- `SW_12V -> separately fused white/red LED modules`; LEDs return to battery negative. LED modules must tolerate AGM charging voltages (about 9–16V specification).
- `BAT_POS -> charger-branch fuse -> charging jack CENTER`. `BAT_NEG -> charger jack SLEEVE`. This branch **bypasses** DPDT main switch. Attach only a compatible AGM/SLA charger, never an unregulated adapter.
- All logic and sensor GND -> common reference `BAT_NEG`, with appropriate routing; no motor current through a logic ground lead.

## D1 Mini inputs

- `THROTTLE_N`: D0/GPIO16 -> external 10k to 3.3V; original momentary NO switch from `THROTTLE_N -> LOGIC_GND`. Released HIGH, pressed LOW. (GPIO16 cannot rely on a normal internal pullup.)
- `CHARGER_PRESENT`: D7/GPIO13 -> 10k to 3.3V; jack **verified** NC sleeve-sense contact to GND when UNPLUGGED; OPEN with plug inserted. D7 low = no plug, high = plug inserted. The extra jack contact is not necessarily isolated. **Do not connect jack-positive to GPIO.** If your jack differs, halt and remap after checking its actual schematic.
- `DIRECTION_ADC`: A0 -> 10k to `LOGIC_3V3` and separately 10k to GND. DPDT isolated direction pole COM -> GND, forward throw -> 1k -> A0, reverse throw -> 10k -> A0. Forward ≈0.27V (~85 counts), reverse ≈1.09V (~337), OFF ≈1.63V (~504). These are approximate because D1 mini includes its own A0 divider. Software only accepts explicit F and R reading windows; all others invalid. Verify your clone A0 voltage rating and resistor tolerances.
- `D4/GPIO2 -> ONEWIRE_DATA`: each DS18B20 data wire joins here; 4.7k pullup to 3.3V, power each probe normally at 3.3V (not parasite mode). D4 **must stay high on boot**; onboard LED is tied to GPIO2.
- `D2/GPIO4 -> I2C_SDA`, `D1/GPIO5 -> I2C_SCL`; OLED + INA226 both powered from 3.3V-compatible supplies, no 5V I2C pullups. Inspect total bus pullups.

## D1 Mini outputs directly to motor driver

- `D8/GPIO15 -> IBT2 R_EN AND L_EN`, with a 10k pulldown to GND physically near driver. **GPIO15 must be low at boot.** Never permanently strap enables to 5V.
- `D5/GPIO14 -> IBT2 RPWM` with 10k pulldown to GND; `D6/GPIO12 -> IBT2 LPWM` with 10k pulldown to GND.
- D3 unused; RX/TX preserved for USB flashing and serial logs.
- `IBT2 GND -> LOGIC_GND`, VCC from its documented logic supply. Do not wire controller GPIOs to motor B+/B-/M+/M-.
- IBT-2 boards frequently incorporate **74HC244**, which when supplied 5V may NOT recognize 3.3V as a guaranteed HIGH. Without level shifting, verify your particular unit and consider a different 3.3V-input-compatible driver if the test fails. The BOM does NOT silently reintroduce a buffer.
- With BOTH enables LOW, bridge should inhibit/coast on typical IBT-2 topology. When enabled and PWM=0, driver behavior may be braking; firmware explicitly lowers both enables on release. Confirm on purchased module.

## INA226 with external high-side shunt

- Shunt Kelvin `IN+` -> `EXTERNAL_SHUNT_IN`, `IN-` -> `EXTERNAL_SHUNT_OUT` (not motor outputs).
- INA226 `VBUS` pin, **if exposed and required**, -> `EXTERNAL_SHUNT_OUT`. Confirm onboard wiring on your exact module first; many modules include a small built-in shunt. Do not pass motor supply current through the breakout's copper, or leave onboard shunt in parallel with the external shunt.
- INA226 VCC -> 3.3V, GND -> LOGIC_GND, SCL/SDA -> I2C lines. Expected I2C address 0x40.
- At 50A, 1.5mΩ shunt drop is 75mV, dissipating 3.75W. Shunt rating, mounting and ventilation must match load.

## Power-on and signal notes

- ESP8266 D1 Mini logic is 3.3V; USB can backfeed some board revisions while external 5V is also present. Test/program with driver power disconnected or appropriately isolated.
- INA226 bus supply readings are NOT necessarily motor winding current under PWM. 50A shunt and 10A fuse are not synchronized current limits.
- With DPDT OFF, display and ESP are normally unpowered and charger operates directly on battery.
- No software interlock can stop a shorted H-bridge transistor or a stuck-high MCU output. This is the tradeoff accepted in revision C.
