"""Repo and UI smoke checks requiring only Python standard library."""
from pathlib import Path
from html.parser import HTMLParser
from xml.etree import ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
README = (ROOT / 'README.md').read_text()
UI = (ROOT / 'include/web_ui.h').read_text()
MAIN = (ROOT / 'src/main.cpp').read_text()
LICENSE = (ROOT / 'LICENSE').read_text()
assert 'GNU GENERAL PUBLIC LICENSE' in LICENSE and 'Version 3' in LICENSE
assert '![Wiring diagram' in README
assert (ROOT / 'docs/diagrams/wiring-overview.svg').is_file()
for item in ('ramp', 'forward', 'reverse', 'motor-rom', 'motor-cut', 'gear-rom', 'gear-cut', 'hold', 'stop', 'emergency-a', 'derate-a', 'derate-s', 'derate-min', 'current-chart', 'derate-cap', 'stall-a', 'stall-s', 'no-load-a', 'no-load-s', 'capacity-ah', 'battery-full', 'battery-empty'):
    assert f'id="{item}"' in UI, item
for route in ('/api/config', '/api/status', '/api/drive', '/generate_204', '/hotspot-detect.html'):
    assert route in MAIN, route
for feature in ('color-scheme:dark', 'pointercancel', 'visibilitychange', 'X-Quad-Control'):
    assert feature in UI, feature
assert 'platform = espressif8266' in (ROOT / 'platformio.ini').read_text()
print('repo, license, embedded image and dashboard checks passed')

assert 'battery_full_v' in MAIN and 'battery_empty_v' in MAIN
assert '/api/battery/reference' not in MAIN
assert 'battery-reference' not in UI

# All wiring remains in one integrated, editable SVG.
svg = (ROOT / 'docs/diagrams/wiring-overview.svg').read_text()
ET.fromstring(svg)
for component in ('12V AGM BATTERY', '3-POSITION DPDT', '50A / 75mV', 'HW-039 / IBT-2', 'LM2596 BUCK', 'LIGHTS', '2-WIRE CHARGER JACK', 'INA226 CURRENT', 'OLED I²C', '2× DS18B20', 'ESP8266 D1 MINI'):
    assert component in svg, component
for sense in ('IN+', 'IN−', 'VBUS', 'K+', 'K−', 'R_EN', 'L_EN', '3V3', '5V', 'GND', 'RPWM', 'LPWM'):
    assert sense in svg, sense
