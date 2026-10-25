# Quad Controller: ESP8266 Ride-On ATV

[![Firmware](https://github.com/iragm/atv/actions/workflows/firmware.yml/badge.svg)](https://github.com/iragm/atv/actions/workflows/firmware.yml)
![License: GPL-3.0](https://img.shields.io/badge/license-GPL--3.0-blue)

A bench-testable **ESP8266 D1 mini + IBT-2/BTS7960** controller for an experimental 12V AGM conversion of a 6V ride-on ATV. It supports a physical throttle, Wi-Fi captive-portal dashboard, adjustable acceleration and voltage-equivalent limits, DS18B20 temperature derating, INA226 telemetry and optional OLED. **Motor outputs are enabled in this firmware**, not a simulator. Design revision **E**, firmware **v0.3**.

The **74AHCT02, 74AHCT125 and MCP23008** are removed; no new active logic or discrete-transistor interlock is added. The direction switch uses an analog resistor ladder, and the ESP8266 drives motor enable and PWM signals directly.

## Wiring overview

![Wiring diagram of the battery, motor driver, ESP8266 and optional sensors](docs/diagrams/wiring-overview.svg)

This embedded **SVG is an image**, and can also be edited directly as vector graphics. It is a **topology overview**, not a native Fritzing `.fzz` breadboard project or an electrical-signoff schematic. See the exact [wiring netlist](wiring-netlist.md) and [bench checklist](bench-checklist.md) before applying power. A native Fritzing file remains a separate milestone once physical module footprints are finalized.

## Hardware setup

1. Check the [parts list with component search links](bill-of-materials.md). The original 6V motor, original F/O/R DPDT switch and throttle can be reused after ratings and pinout checks. Set the buck to **5.0V before connecting the D1 mini**.
2. Wire the D1 mini and driver per [wiring-netlist.md](wiring-netlist.md). Keep motor current out of the logic-ground conductors. The INA226, OLED and DS18B20s are **optional** for boot and basic motor operation.
3. Verify the original direction switch **isolates the power feed in center OFF**, and that the A0 ladder reads approximately 85 (forward), 337 (reverse) and 504 (OFF) on a D1 Mini with 0–3.3V ADC input. Adjust the ADC windows in `include/config.h` if your actual measured values differ.
4. **IBT-2 compatibility:** verify its logic inputs recognize 3.3V. Many boards buffer the inputs through a 5V `74HC244`, so this is **not guaranteed** without a level shifter. Do not connect 5V into ESP8266 GPIO.
5. Use a fused, current-limited bench supply and motor disconnected for the first GPIO/charger/sensor tests. Then test the motor under light load. A slow vehicle still has significant stall current and 12V PWM pulses on a 6V-rated motor.

### Pin map

| D1 mini | Function | Notes |
|---|---|---|
| D0 / GPIO16 | Physical throttle | 10k external pullup, active low |
| D1 / D2 | I2C | OLED and INA226 at 3.3V |
| D4 / GPIO2 | DS18B20 bus | 4.7k pullup, pin must be HIGH at boot |
| D5 / D6 | IBT-2 RPWM / LPWM | 10k pulldowns |
| D7 / GPIO13 | Charger-plug sense | 10k pullup, jack NC contact to ground when unplugged |
| D8 / GPIO15 | Both IBT-2 enable pins | 10k pulldown; LOW at boot |
| A0 | Direction resistor ladder | Sensor values F~85 / R~337 / OFF~504 |
| RX / TX | Serial | Debug and USB flash |

## Flash / build

Requires PlatformIO and a standard ESP8266 USB serial driver:

```sh
pio run -e d1_mini                # compile
pio run -e d1_mini -t upload      # flash
pio device monitor -b 115200      # monitor GPIO ADC, sensors and status
```

GitHub Actions builds on pushes, PRs and manual dispatch; the **Firmware** workflow compiles host tests and a `quad-controller-d1-mini.bin` downloadable artifact. See [Actions](https://github.com/iragm/atv/actions/workflows/firmware.yml). Local host tests can also be run with:

```sh
g++ -std=c++17 -Wall -Wextra -Werror -Iinclude tests/control_test.cpp -o /tmp/quad-test && /tmp/quad-test
g++ -std=c++17 -Wall -Wextra -Werror -Iinclude tests/energy_test.cpp -o /tmp/energy-test && /tmp/energy-test
python3 tests/repo_smoke.py
```

## Phone configuration / captive portal

1. Power the controller with the direction switch in **forward or reverse**. The ESP8266 starts an AP named `QUAD-XXXXXX` (suffix is its chip ID).
2. Connect your phone to this AP using the default WPA2 password **`quadsetup26`**. **Change this in `include/config.h` and reflash before regular use.** This is not a secure remote-control system for an untrusted environment.
3. Your phone's captive-portal assistant should open the dashboard. If not, open **http://192.168.4.1/** in a browser. Some phones label this Wi-Fi as *no internet*; choose to stay connected.
4. The dark dashboard displays the same information as the OLED (drive status/direction, output duty, voltage, current, temperatures and warnings), plus peak current, motor power, rolling five-minute current, motor energy used and a qualified runtime estimate. Configure settings and hit **Save**. Settings survive reboots via EEPROM emulation.
5. For remote movement, choose **Forward** or **Reverse** and **hold** the green `HOLD TO DRIVE` button. Release it or hit STOP for immediate Wi-Fi stop. The browser refreshes the command every 180ms and the controller expires it after 600ms without valid updates. **The physical throttle takes priority.** Center-OFF on the hardware direction switch inhibits both inputs.

### Configurable parameters

| Setting | Default | Behavior |
|---|---:|---|
| Ramp time | **2.0s** | Linear ramp from 0 to requested voltage-equivalent limit; 0.1–30s |
| Max forward voltage | **6.0V** | Equivalent mean PWM voltage; adjustable 0.5–12V |
| Max reverse voltage | **4.0V** | Separate reverse ceiling; adjustable 0.5–12V |
| Motor sensor | First discovered DS18B20 | Selected by stable 64-bit device ROM; can be missing |
| Motor high-temp cutoff | **75°C** | Caps output to **5V equivalent** while hot; LCD warns |
| Gearbox sensor | Second discovered DS18B20 | Selected by stable device ROM; can be missing |
| Gearbox high-temp cutoff | **70°C** | Same 5V equivalent cap and LCD warning |
| Physical throttle release grace | **2.0s fixed** | Holds last duty up to two seconds, then releases motor; Wi-Fi release stops immediately |
| Stall current threshold | **0A (off)** | Configurable 0–50A; cuts drive after excessive motor-branch current persists |
| Stall trip delay | **0.7s** | 0.2–5s; repeated high-current samples required, not a one-sample cutoff |
| Possible no-load threshold | **0A (off)** | Configurable 0–20A; *heuristic only*, can cut normal low-load travel; not true wheel-spin detection |
| No-load trip delay | **1.5s** | 0.5–10s; low-current check active only above ~20% PWM |
| Battery rated capacity | **7Ah placeholder** | Enter actual battery rating; label value alone cannot predict high-load AGM usable capacity |
| Session starting charge | **Unknown** | Enter a known approximate charge percentage on the separate dashboard button after charging; RAM-only |

*Voltage-equivalent* is a duty limit: approximately `PWM duty = min(target voltage / measured supply voltage, 1)`. It **does not** generate a true 6V DC waveform: the motor still sees 12V-class PWM pulses. INA226 supply sensing is used when available; otherwise a **nominal 12.0V** is assumed and reported. Thermal limiting overrides either direction setting, never raising a lower configured cap. Unavailable temperature sensors are reported but do not block the controller or falsely trigger derating.

**Software-only protections:** release-to-arm after boot, direction/source change lockout until both controls release, charger-plug inhibit, center-OFF inhibit, physical pedal grace, 600ms Wi-Fi heartbeat expiry. None of these can contain a shorted motor driver. Keep the original fuses and appropriate DC wiring. High-current stall and low-current no-load trips both default to **0 (disabled)** until bench measurements establish credible values. Both need a working INA226. A sustained fault disables PWM and remains latched until the pedal and remote command are released. At low PWM (<20%), these current heuristics are skipped to avoid false trips during startup.

### Battery and stall telemetry, revision E

The external **50A/75mV shunt is on the switched motor branch** in the [existing netlist](wiring-netlist.md). It measures DC input current to the IBT-2. It **does not** directly measure motor-winding current, other vehicle loads, or charger current. The charger is connected on a separate branch, bypassing both the motor shunt and the normal D1 Mini switched power supply. Consequently, **charging current/rate is unavailable in this wiring**; do not interpret the presence of a charger plug or a rising voltage as a measured charging rate.

Live telemetry includes motor input watts (`bus V × input A`), session peak motor current, accumulated **motor-branch** Ah and Wh since boot, and a trailing five-minute current/power average. The remaining runtime estimate is displayed **only** after entering a manually known starting-charge percentage in the **Battery charge reference** section. Enter a new reference after every reboot or major charge. It subtracts monitored motor-branch Ah consumed since that reference, then divides the remaining nominal Ah by average motor current over the last five minutes (minimum 30 seconds of readings, at least 0.1A load). Stationary readings and battery chemistry effects make it approximate; the motor-only sensor excludes accessory loads. AGM batteries deliver less rated capacity at high discharge rates, and terminal voltage under load is not an accurate state-of-charge gauge.

A stalled brushed DC motor typically pulls more current, allowing a sustained over-current cutoff. A freely spinning/unloaded motor can pull *less* current, allowing only an **optional possible no-load warning/cutoff**. From a single current sensor, no software can confidently distinguish wheel spin against the ground from downhill travel, unloaded testing, or a disconnected drivetrain. Proper traction/slip estimation requires added wheel-speed/vehicle-motion sensing. Calibrate both options on a current-limited bench setup, and leave the no-load cutoff off if it causes nuisance trips.

**Current monitor calibration:** The 1.5mΩ shunt yields 75mV at 50A, close to the INA226's ~81.92mV full-scale range. The shunt dissipates 3.75W at 50A. Confirm the shunt rating, Kelvin sense wiring, ESP power arrangement and the motor fuse before setting trip thresholds. The current monitor can miss a very fast current spike because of conversion averaging; software monitoring is not a substitute for a properly sized fuse.

## Repository contents

```text
.github/workflows/firmware.yml   Build, host checks and downloadable binary
src/main.cpp                    Firmware, REST API, captive DNS and sensors
include/control.h              Host-tested motor control logic
include/energy.h               Five-minute load telemetry and fault detection
include/settings.h             Persistent configuration schema, v2 migration and checksum
include/web_ui.h               Tasmota-inspired dark mobile dashboard
include/config.h               GPIO mapping and fixed device parameters
platformio.ini                 ESP8266/D1 Mini build dependencies
tests/                         Unit tests and repository/UI smoke checks
docs/diagrams/                 Editable SVG image and optional PNG export
firmware-spec.md               Feature and behavior specification
wiring-netlist.md              Electrical connections
bill-of-materials.md           BOM and component search links
bench-checklist.md             Test plan
LICENSE                        GNU GPL version 3
```

## License

**GNU General Public License version 3 (GPL-3.0-only)**. See [LICENSE](LICENSE). Keep the corresponding source and license with redistributions.
