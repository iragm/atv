# Adversarial code review, revision F

**Scope:** source-level review and host tests of firmware v0.4. This review was conducted by the assistant in this conversation; **it is not an independently executed Astra review**. Verify actual hardware behavior on a current-limited bench supply.

## Review findings and changes

1. **Previous current trips masked small-duty motor current.** Emergency trip now checks the first *valid current sample* at any nonzero motor output; stall/no-load heuristics continue to ignore startup duty under ~20%. Controller explicitly distinguishes `EmergencyCurrent`, `Overcurrent`, and `NoLoad`.
2. **Overload detection only offered full cutoff.** Added a sustained-current state machine that delays action, then lowers permitted PWM equivalent voltage at 2V/s to a configurable floor. Recovery requires motor current to fall below 90% threshold and rises at 1V/s. A temperature derate and a user voltage cap remain upper bounds.
3. **No protection against existing EEPROM layout changes.** Added `v3 -> v4` migration, preserving calibration and leaving new trip thresholds off until bench measurement. Existing v2 migration remains.
4. **Settings could be changed mid-drive.** Configuration writes return HTTP 409 while the motor is active or either throttle is commanded. Saving also resets the current derating state, *not* any latched trip.
5. **Timebase and sample reliability.** Added tests for sustained spikes, interruption resets, delayed derating, floor, recovery, low PWM emergency trip, missing data, and `millis()` wraparound. Current sample period reduced to nominal 25ms. Firmware is event-loop based, so this is not a worst-case timing guarantee.
6. **Potentially misleading “instantaneous stall” wording.** Firmware cannot prove a mechanical stall using one motor-branch current monitor. Dashboard and documentation call the first-sample event an **emergency overcurrent cutoff** and use separate sustained stall detection.
7. **Network and security boundary.** Firmware still hosts a local HTTP captive portal behind a hardcoded WPA2 password. The `X-Quad-Control` header is **not** authentication. Any AP-connected client may control the motor and edit settings. Unique passphrase and physical center-OFF remain essential before real use.

## Known residual risks and limits

- The INA226 samples shunt drop on the IBT-2 supply branch. No motor-winding phase current, wheel rotation, traction, or charge current is sensed.
- The INA226's sampling, conversion/averaging, I2C polling and blocking operations mean *first sampled* does not mean electrically immediate. A fuse/current-limited supply remains necessary for wiring and driver faults.
- Loss of INA226 disables current-based protective functions; already-latched trips remain latched. This is deliberate degraded-mode operation, surfaced in web status.
- ADC faults, bad current-shunt calibration, driver input threshold mismatches, an internally shorted power stage, brownouts, or an ESP8266 reboot are not mitigated by software current limits.
- A stall may draw less *DC input* current under PWM than a naive steady-state prediction. Calibrate all thresholds from actual loaded measurements and consider false positives from inrush.
- External independent Astra code review and physical acceptance have not occurred.

## Verification checklist

- [x] Controller and settings tests compile under `-Wall -Wextra -Werror` and pass.
- [x] Load-protection and energy tests compile and pass.
- [x] Web UI JavaScript syntax check via `node --check`.
- [x] README/embedded-assets smoke checks pass.
- [ ] PlatformIO ESP8266 compile in GitHub Actions.
- [ ] Download and inspect built firmware artifact.
- [ ] Real-board INA current calibration, current threshold selection, USB/ADC and direction testing.
- [ ] External independent Astra review.
