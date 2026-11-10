#!/usr/bin/env python3
"""Guard the strategy quad helper against unnecessary custom geometry."""

from pathlib import Path
import re
import sys

core = (Path(__file__).resolve().parents[1] / "src/lens-core.c").read_text()
start = core.index("void lens_draw_quad")
end = core.index("// -------------------------------------------------------------------------------------------------", start)
body = core[start:end]

errors = []
if "gs_draw_sprite(NULL, 0, cx, cy);" not in body:
    errors.append("strategy quad must use OBS gs_draw_sprite helper")
for forbidden in ("gs_render_start(", "gs_texcoord(", "gs_vertex2f(", "gs_render_stop("):
    if forbidden in body:
        errors.append(f"manual quad geometry reintroduced: {forbidden}")

if errors:
    for error in errors:
        print("FAIL:", error)
    sys.exit(1)

print("PASS: custom-effect passes use OBS's sprite quad helper")
