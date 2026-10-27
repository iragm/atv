"""Repo and UI smoke checks requiring only Python standard library."""
from pathlib import Path
from html.parser import HTMLParser

ROOT = Path(__file__).resolve().parents[1]
README = (ROOT / 'README.md').read_text()
UI = (ROOT / 'include/web_ui.h').read_text()
MAIN = (ROOT / 'src/main.cpp').read_text()
LICENSE = (ROOT / 'LICENSE').read_text()
assert 'GNU GENERAL PUBLIC LICENSE' in LICENSE and 'Version 3' in LICENSE
assert '![Wiring diagram' in README
assert (ROOT / 'docs/diagrams/wiring-overview.svg').is_file()
for item in ('ramp', 'forward', 'reverse', 'motor-rom', 'motor-cut', 'gear-rom', 'gear-cut', 'hold', 'stop', 'emergency-a', 'derate-a', 'derate-s', 'derate-min', 'current-chart', 'derate-cap', 'stall-a', 'stall-s', 'no-load-a', 'no-load-s', 'capacity-ah', 'battery-reference'):
    assert f'id="{item}"' in UI, item
for route in ('/api/config', '/api/status', '/api/drive', '/api/battery/reference', '/generate_204', '/hotspot-detect.html'):
    assert route in MAIN, route
for feature in ('color-scheme:dark', 'pointercancel', 'visibilitychange', 'X-Quad-Control'):
    assert feature in UI, feature
assert 'platform = espressif8266' in (ROOT / 'platformio.ini').read_text()
print('repo, license, embedded image and dashboard checks passed')
