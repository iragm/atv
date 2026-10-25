# Design audit and migration notes

## Starter ZIP

Imported 6 Markdown files from the supplied `quad_controller_project_starter.zip` (Oct 23, 2026). Original revision B referred to hardware gate 74AHCT02, PWM buffer 74AHCT125, direction/charge expander MCP23008. The original visual Fritzing-style illustration was *not present* as an editable `.fzz` file or as an image in the ZIP.

## Revision C, hardware request

- **74AHCT02 removed.** `D8/GPIO15` now directly controls both IBT-2 enables. Throttle and charging are gated only in firmware; 10k enable pull-down remains a passive boot-state default, not an external transistor interlock.
- **74AHCT125 removed.** `D5` and `D6` now connect to RPWM/LPWM. Potential issue: 5V 74HC244 on some IBT-2 boards does not guarantee recognizing 3.3V input HIGH. Cannot eliminate that IC AND promise unverified electrical compatibility. Validate actual board or replace driver with one that guarantees 3.3V inputs.
- **MCP23008 removed.** Direction DPDT isolated pole becomes resistor ladder on `A0`, preserving USB RX/TX. Charger-sense signal uses D7. The new analog approach cannot separately identify a simultaneous F+R contact short with the same certainty as two discrete inputs; invalid intermediate ADC levels are rejected.
- **No transistors or replacement active logic** were inserted.
- `D4/GPIO2` carries the DS18B20 bus with the mandatory pullup, which also satisfies its high-on-boot requirement. D8/enable external pulldown satisfies low-on-boot. D3 left untouched.

## Design validation items

1. Confirm exact switch/jack contacts with multimeter. Jack-switch extra pin may connect to sleeve, not independent contact; never attach 12V to D7.
2. Verify D1 Mini analog input limits and calibrate ADC direction resistor-ladder ranges.
3. Check IBT-2 schematic / 74HC244 / logic VCC. 3.3V PWM HIGH is not guaranteed at 5V 74HC244 VCC.
4. Confirm actual current monitor shunt hookup; motor current never through tiny INA226 PCB traces.
5. Original motor is 6V-rated, while the proposed battery is 12V. 30% average PWM does not turn 12V pulses into equivalent 6V-only electrical stress.
6. Verify fuse current ratings and wire gauge from real load current, not the 43A chip marketing rating.
7. With only software inhibit, motor control can fail if firmware hangs, GPIO sticks high or bridge transistors short. Remaining main power switch is the physical power disconnect.
8. Smart AGM charger remains connected directly to battery behind its own fuse, regardless of main switch position; OLED dark when main is OFF.
9. The wiring-overview SVG is not a native Fritzing project. Make `.fzz` after receiving precise module identification/footprints.
10. Motor-drivetrain thermals, sensor mapping and battery low-voltage levels must be measured, not copied from generic internet recommendations.
