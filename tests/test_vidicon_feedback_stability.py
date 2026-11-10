#!/usr/bin/env python3
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
shader = (ROOT / "data/emulens.effect").read_text(encoding="utf-8")

trail = re.search(
    r"float4 PSVidicon\([^)]*\)\s*:\s*TARGET\s*\{(?P<body>.*?)\n\}",
    shader,
    re.S,
)
composite = re.search(
    r"float4 PSVidiconComposite\([^)]*\)\s*:\s*TARGET\s*\{(?P<body>.*?)\n\}",
    shader,
    re.S,
)

errors = []
if not trail or not composite:
    errors.append("Vidicon shader functions are missing")
else:
    t = trail.group("body")
    c = composite.group("body")

    if "float3 feedback_rgb = max(injected, retained);" not in t:
        errors.append("Vidicon no longer stores full-frame replacement feedback")
    if "return float4(feedback_rgb + guard, current.a);" not in t:
        errors.append("Vidicon history must preserve a full opaque frame")
    if "float3 final_rgb =" not in c or "lerp(feedback.rgb, registered" not in c:
        errors.append("composite no longer presents the full feedback frame")
    if "max(original.rgb, trail_rgb)" in c:
        errors.append("sparse trail-vs-current composite was reintroduced")
    if "return float4(current.rgb + guard, current.a);" not in t:
        errors.append("zero-decay path should return the current frame")
    if "stored_excitation" not in t or "vid_threshold" not in t:
        errors.append("highlight threshold no longer shapes persistence retention")

if errors:
    for error in errors:
        print("FAIL:", error)
    sys.exit(1)

print("PASS: Vidicon preserves full-frame tube feedback for real-footage persistence")
