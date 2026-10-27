# Firmware specification, revision F / v0.4

## Goals

ESP8266 D1 Mini actively commands the motor driver in bench and normal use. Use an offline WPA2 access point with captive portal rather than relying on the internet or a home Wi-Fi network. The web dashboard and optional OLED render the same underlying state. Missing peripherals do not crash or prevent control.

## Firmware modules

- `include/control.h`: pure C++ ramp, source arbitration, release hold, charger/direction/fault inhibits and temperature-limited voltage-to-PWM math. Uses integer millisecond clocks and handles wraparound deltas.
- `include/settings.h`: versioned EEPROM data, fields and FNV-1a integrity checksum; valid v2 or v3 settings migrate to v4, invalid settings restore defaults.
- `include/energy.h`: pure C++ 300-second motor-branch current/power rolling history, Ah/Wh and configurable sampled emergency cutoff, sustained-current PWM derating, continuous-current fault latches. No RTC or filesystem.
- `src/main.cpp`: inputs, GPIO PWM, I2C INA226, OneWire DS18B20, U8g2 OLED, DNS server, HTTP routes and persistent settings.
- `include/web_ui.h`: one self-contained dark mobile webpage stored in program flash; no CDN, SPIFFS/LittleFS or live internet required.

## AP / HTTP contract

- Always-on soft AP `QUAD-<chip-id>`, WPA2 password currently fixed in `config.h`. Captive wildcard DNS and redirects for typical Apple/Android/Windows HTTP probes. Landing page at `http://192.168.4.1/`; OS popup is best-effort, direct URL is deterministic.
- `GET /api/status`: JSON state, duty %, source, direction, battery V/A, thermals, faults, probe list and found/missing flags.
- `GET /api/config`: JSON settings and detected DS18B20 ROM IDs.
- `POST /api/config`: URL-encoded fields `ramp_s`, `forward_v`, `reverse_v`, `motor_sensor`, `gearbox_sensor`, `motor_cutoff_c`, `gearbox_cutoff_c`, `emergency_trip_a`, `derate_trip_a`, `derate_trip_s`, `derate_minimum_v`, `stall_trip_a`, `stall_trip_s`, `no_load_below_a`, `no_load_trip_s`, `battery_capacity_ah`; validates ranges and checksum then writes flash only when Save is pressed. A controller header `X-Quad-Control: 1` is required.
- `POST /api/battery/reference`: accepts `percent=0..100` as a RAM-only starting-charge reference; clears prior consumption baseline; use only when the battery charge estimate is known.
- `POST /api/drive`: `hold=1|0&direction=fwd|rev`, same header required, heartbeats while held. 600ms command TTL with physical-input priority. No cached throttle command survives reboot.
- Endpoint permissions are only protected by the AP WPA2 boundary and a non-secret same-origin browser header. No TLS or user auth; do not use near untrusted clients. Use a unique AP password before service.

## Motion state machine

1. Hardware F/O/R analog ladder must be in a valid energized position for **any** drive command; center OFF disables motion.
2. At boot or after a fault, no movement until a released throttle has been observed.
3. Physical pedal takes precedence over remote held button. While engaged, duty rises linearly over `ramp_s` to `min(configured voltage, 5V while high temperature)/measured Vbus * 1023`, clipped at 1023. If INA226 is absent, use 12.0V and report the assumption.
4. When the physical pedal is released, hold the last output duty for **up to 2000ms**, unless charger, invalid switch, fault, or source/direction change occurs. Press within the grace period to resume ramp progression. After the grace period expires, set both PWM outputs zero and disable both IBT-2 enables.
5. A Wi-Fi hold command bypasses the physical grace period: releasing the web button stops immediately; losing heartbeats for 600ms stops. Switching directions or handoff between pedal and web control while driving demands release-to-arm, not a hot reverse.
6. Motor and gearbox over-temp thresholds independently cap output to 5V equivalent and show warnings. If a selected probe is absent or reports invalid data, warn and **do not** derate from that missing probe. This is deliberate degraded mode, not strict fail-safe mode.
7. Charger plugged, invalid hardware switch or any latched current trip cuts output immediately on the *next control step* (and forces motor disable from the INA sample handler when sampled). Current monitor normally polls every 25ms, not an electrically guaranteed maximum. Emergency cutoff applies to the **first valid sample over its threshold at any nonzero PWM**. The sustained stall trip and optional no-load trip require duty >=205/1023 (~20%); default disabled (0A); trip delays default 700ms / 1500ms. Current-based derating begins after current exceeds a configurable threshold (default disabled) for 700ms. Its equivalent-voltage cap falls 2V/s to an adjustable floor (default 2V), recovers 1V/s once current falls below 90% of threshold and resets on throttle stop. Thermal cap and user voltage caps still apply. Trips latch until command release. Invalid/missing INA readings deactivate all current-based safeguards but preserve already-latched trips; the UI warns about this degraded mode. These are DC motor-input current heuristics, not winding-current or traction sensors.
8. Session motor-branch current/power, peak, Ah and Wh update from ~25ms sensor polling. The 300-second rolling average includes idle; missing samples are unknown rather than fabricated. Capacity defaults to an unverified 7Ah placeholder, editable by user. Runtime depends on a manually supplied starting-charge percentage (RAM-only each boot) and is hidden until >=30s of valid samples and >=0.1A average draw. It excludes LED, controller and charger loads, Peukert losses, and any battery usage during power-off.
9. The charger bypasses the motor-branch shunt and the switched ESP power feed; true charger amps/charge rate cannot be measured with this wiring. Changing that is a separate hardware wiring design decision.

## Persistence

Default ramp 2 seconds; max forward 6V; max reverse 4V; motor sensor 1st ROM; gearbox sensor 2nd ROM; thresholds 75°C/70°C. Selectors store probe ROMs, not enumeration positions. An all-zero ROM means auto fallback based on detected order. A saved ROM that disappears stays selected and is reported missing. Values are validated on write and load. V2 and V3 settings migrate to V4 in RAM; saved V4 only when Save is pressed. All current protection thresholds start disabled; charge reference and energy counters are intentionally volatile. Repeated telemetry polling never writes flash.

## Peripheral degradation

| Missing component | Expected behavior |
|---|---|
| OLED | Skip screen initialization and rendering, web control works |
| Both DS18B20 | Temperature readouts unavailable; no thermal derate |
| One DS18B20 | Available probe works; missing assignment is disclosed |
| INA226 | Voltage/current/energy/runtime show unavailable; all current trips and current derating are inactive; limit calculation assumes 12.0V |
| Phone/Wi-Fi connection | Physical throttle works; remote expires |

## CI / verification

- Host compiler tests energy integration, missing sensors, 300s rolling history, emergency first-sample cutoffs, overload derating/recovery, stall and no-load timers, and state changes, grace timing, reverse, charging, thermal cap, settings checksum/validation and direction ladder.
- Python smoke test verifies README image/links and dark UI/expected controls.
- GitHub Actions builds `d1_mini` with PlatformIO, uploads `.bin`, and runs host tests on every push and PR.
- Hardware acceptance pending: confirm 3.3V driver logic thresholds; verify joystick/DPDT ladder; measure INA accuracy against real shunt and supply; validate power-up/boot GPIO; thermal tests with probes warmed; Wi-Fi disconnect/reconnect and captive prompt on iOS/Android; real motor current and fuse behavior.

## Not yet implemented

- Authenticated Wi-Fi provisioning, TLS, OTA, Wi-Fi disable while riding, low-voltage cutoff, detailed datalogging, NTP, comprehensive per-fault history and a native Fritzing `.fzz` editable breadboard diagram. None are prerequisites for the current user-specified firmware build.
