# Revision C bill of materials

See the [README parts table](README.md#parts-list-and-component-search-links) for click-to-search component links.

**Removed**: one 74AHCT02, one 74AHCT125, one MCP23008. **Not substituted with another active component.** The original NO throttle switch, existing DPDT F/O/R switch, charger sleeve-switch contact, and generic resistors connect directly to the D1 Mini.

## Core electronics

- 1 Wemos/LOLIN D1 Mini (ESP8266); preserve RX/TX for debugging.
- 1 IBT-2/BTS7960 H-bridge; identify its logic buffer IC and test 3.3V input compatibility.
- 1 INA226 I2C current monitor module capable of Kelvin-sensing an external 1.5mΩ shunt.
- 1 external 50A/75mV (1.5mΩ) rated shunt. Must handle thermal dissipation.
- 1 0.96in 128x64 I2C OLED, SSD1306 (default firmware) or SH1106 (configuration).
- 2 DS18B20 probes; identify motor and gearbox sensor ROM IDs before labeling them as such in software.
- 1 LM2596-type adjustable 12V-to-5V buck module with adequate supply current and filtering.
- 1 12V AGM battery of verified dimensions/capacity; 1 appropriate AGM smart charger.
- 1 verified NC-sleeve switched barrel jack center-positive compatible with charger.
- Existing isolated-pole DC-rated DPDT center-off F/O/R and existing NO momentary throttle.
- 2 white and 2 red LED modules rated for 9–16V automotive service.

## Passive and wiring consumables

- Direction A0 divider: **2×10k** (3.3V/A0/GND rails), **1×1k** (forward contact), **1×10k** (reverse contact).
- Throttle and charger inputs: **2×10k pullups** to 3.3V.
- IBT-2 enables/PWM lines: **3×10k pull-downs** to GND (one EN shared, one RPWM, one LPWM).
- 1-Wire line: **1×4.7k** pullup to 3.3V.
- Adequate motor and branch fuse holders, grounded appropriately, heatshrink, strain relief, insulated terminals and correctly rated wiring.
- Suggested filter capacitors: 470–1000µF at ≥25V near B+/B- and 47–100µF at ≥10V near buck 5V output, plus ceramic 100nF decouplers as supported by module schematics.

## Fuse and electrical-rating placeholders

- Motor branch: existing 10A blade fuse **provisional** pending stall/start-current measurements, wire gauge and switch ratings.
- Charger branch: example ~2A for ~1A charger pending actual charger and cable ratings.
- Control branch: example ~1A pending actual electronics draw/wire rating.
- LED branch: size for actual LED load and wires.

**Do not infer a final fuse rating from the IBT-2's advertised 43A, and do not assume the original 6V motor is rated for 12V.**
