# Rev E bench checklist (software-only inhibit and INA226 load telemetry)

## Power and inputs, without motor

- [ ] Verify actual D1 Mini ADC `A0` accepts up to 3.3V external; check supply while USB-powered.
- [ ] Verify DPDT F/O/R is center-off, poles isolated, rated for required 12V DC and breaks before makes.
- [ ] Verify throttle NO, pullup to 3.3V and ground path (press active LOW).
- [ ] Verify switched barrel jack sleeve NC contact is grounded when empty, open when plugged; plug **positive must never reach D7**.
- [ ] Verify charger output/polarity/chemistry and independently fused direct-battery branch.
- [ ] Confirm FWD A0 ~85, REV ~337, OFF ~504 ADC counts; tweak firmware windows when tested.
- [ ] Confirm D4 (1-Wire) stays HIGH on boot; D8 remains LOW on boot/reset; D5/D6 PWM inputs held low by resistors before boot.
- [ ] Adjust LM2596 to 5.0V with the D1 disconnected; verify logic 3.3V and IBT-2 VCC requirements.
- [ ] Inspect exact IBT-2 logic IC, verify whether 3.3V HIGH is recognized **reliably** at 5V VCC (not just one lucky threshold crossing).
- [ ] Verify USB/external-power arrangements do not backfeed damaging voltages.

## Logic tests, motor disconnected

- [ ] Boot with pedal released: READY, enables low and PWM=0 until press.
- [ ] Boot with pedal held: WAIT_RELEASE, enables low until release and new press.
- [ ] Hold pedal in F: duty ramps from zero to forward cap. Release: last PWM held for up to **2 seconds**, then EN low and both PWM zero. Tap the pedal within the hold window and confirm the ramp resumes.
- [ ] Change to R while held: no reverse until a release and subsequent press.
- [ ] Insert charger plug while driving: EN low. With charger connected, pedal press cannot drive.
- [ ] Disconnect and reconnect charger while holding pedal: no automatic re-arm.
- [ ] Direction invalid (center OFF while USB-powered / ladder disconnected): motor inhibited.
- [ ] Confirm no overlap of RPWM and LPWM on scope during transitions.
- [ ] Confirm INA226 shunt current and bus voltage against a meter; disconnect INA226 and verify telemetry becomes unavailable while physical driving still works. No current-dependent cutoffs should trip with the sensor missing.
- [ ] Confirm OLED controller choice and display values; disconnect DS18B20 probes and inspect diagnostics.

## Bench motor testing

- [ ] Secure quad with driven wheels unloaded before powering driver; use current-limited bench source where practicable.
- [ ] Check direction and PWM response at low duty, verify both enable pins have good levels.
- [ ] Verify correct motor coast/brake behavior when EN is low for your IBT-2 board.
- [ ] Capture normal unloaded, flat-ground, starting and near-stall current with a calibrated meter. Only then enable calibrated derate, timed stall and emergency trip thresholds; test both momentary inrush (should not trip) and sustained high current (must disable motor output and require throttle release).
- [ ] Test first-sample emergency cutoff with an artificial above-threshold current reading at low PWM, and verify PWM and EN go LOW. Account for INA conversion and application-loop latency.
- [ ] Test derating for >configured delay: voltage-equivalent cap falls steadily, never below the configured floor. Confirm recovery below 90% threshold and reset after throttle release.
- [ ] Test the optional low-current/no-load detector first with wheels unloaded. Beware normal low-load motion, downhill travel, and false positives. Disable it if traction cannot be inferred reliably from the observed currents.
- [ ] Verify the 5-minute current average integrates idle intervals, the peak capture and motor-branch Ah/Wh against an external wattmeter.
- [ ] Enter a known approximate battery charge reference in the portal and verify runtime estimate appears only after at least 30s of valid readings, becomes unavailable near zero draw, and is reset after power cycling.
- [ ] Confirm charger current is explicitly **unavailable**: charger branch bypasses the shunt and D1 Mini is off with direction switch OFF.
- [ ] Capture startup/stall current and heat rise, then choose appropriate fuse/wiring, current-trip/thermal limits and motor duty caps.
- [ ] Test brownout/reboot while motor command was active and ensure post-reboot pedal release is needed.
- [ ] Check DPDT power switch breaking loaded current and wiring/heatsink temperatures.
- [ ] Test charger jack insertion detection with charger unpowered and powered; charging remains independent with F/O/R OFF.
- [ ] Record evidence/photos and scope captures in `docs/bench-results/` once available.

The removed gate means protection against faulty firmware or failed driver is out of scope. The small 2mph top speed does not bound stalled motor current, wiring heating or battery short-circuit energy.
