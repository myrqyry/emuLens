#!/usr/bin/env python3
"""Verify the Wet Lens cell search covers maximal legal droplet footprints."""
import re
from pathlib import Path

source = (Path(__file__).resolve().parent.parent / "data/emulens.effect").read_text()
shader = source.split("float4 PSWetLens(", 1)[1].split("technique DrawWetLens", 1)[0]
loops = {}
for axis in "xy":
    variable = "cy" if axis == "y" else "cx"
    match = re.search(r"for\s*\(int\s+" + variable + r"\s*=\s*(-?\d+)\s*;\s*" + variable + r"\s*<=\s*(-?\d+)", shader)
    assert match, f"Missing {axis} search loop"
    loops[axis] = tuple(map(int, match.groups()))

# At max size 1.25 and stretch 0.8, a 0.58-cell radius becomes
# 0.725 cells before stretching; centers vary between 0.18 and 0.82.
radius = (0.13 + 0.45) * 1.25
vertical_reach = radius * (1 + 0.8 * (0.28 + 0.58))
# Horizontal remapping has a minimum divisor of 0.65.
horizontal_reach = radius / 0.65
for axis, reach in (("x", horizontal_reach), ("y", vertical_reach)):
    low, high = loops[axis]
    assert low <= -2 and high >= 2, (
        f"{axis} coverage {low}..{high} misses cells two away "
        f"at maximal footprint {reach:.3f}"
    )
print("PASS: Wet Lens searches all neighbors at maximum controls")
