"""
Contract test for the emulens_color_to_vec4 helper in lens-params.h.

Locks in the OBS-canonical byte order for color values produced by
obs_properties_add_color_alpha. OBS stores the int with little-endian
byte order: bytes [R, G, B, A] (so the int reads as 0xAABBGGRR on
little-endian platforms — see libobs/graphics/vec4.h vec4_from_rgba
which uses memcpy for the same reason).

The helper must:
  - Use memcpy (or equivalent) to extract channels — NOT bit shifts.
    Bit shifts against the wrong byte-order assumption caused a
    red↔blue swap that was visible in starburst, bloom, vidicon,
    polaroid, and leak color settings.
  - Output (R, G, B, A) in [0,1] for white (0xFFFFFFFF) and red
    (0xFFFF0000 stored as 0xFF0000FF in int form? No — stored as
    0xFF in R position, so byte[0]=0xFF means red).

Failing this test means the helper was changed to use bit shifts
or its byte-order assumption was inverted.
"""

import os
import re
import sys

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
HEADER_PATH = os.path.join(PROJECT_ROOT, "src", "lens-params.h")


def fail(msg):
    print(f"FAIL: {msg}")
    return 1


def main():
    if not os.path.exists(HEADER_PATH):
        return fail(f"{HEADER_PATH} not found")

    with open(HEADER_PATH, "r", encoding="utf-8") as f:
        text = f.read()

    failures = 0

    # Must use memcpy (portable, host-byte-order safe)
    if "memcpy" not in text or "emulens_color_to_vec4" not in text:
        failures += fail(
            "emulens_color_to_vec4 must be defined and use memcpy "
            "for byte-order-safe channel extraction"
        )

    # Find the helper body
    m = re.search(
        r"static inline void emulens_color_to_vec4\s*\([^)]*\)\s*\{(.*?)\n\}",
        text,
        re.DOTALL,
    )
    if not m:
        failures += fail("could not locate emulens_color_to_vec4 body")
        print(f"FAILED: {failures}")
        return 1

    body = m.group(1)

    # Must NOT use bit shifts — those produced the red↔blue swap bug
    if re.search(r">>\s*\d+\s*&\s*0xFF", body):
        failures += fail(
            "emulens_color_to_vec4 uses bit-shift extraction; "
            "OBS stores color as little-endian bytes [R,G,B,A], so "
            "shifts against 0xAARRGGBB swap R and B"
        )

    # Must reference all 4 bytes (R, G, B, A)
    for ch in ("bytes[0]", "bytes[1]", "bytes[2]", "bytes[3]"):
        if ch not in body:
            failures += fail(f"emulens_color_to_vec4 missing channel: {ch}")

    if failures:
        print(f"FAILED: {failures}")
        return 1

    print("PASSED: emulens_color_to_vec4 matches OBS contract (LE bytes, R/G/B/A)")
    return 0


if __name__ == "__main__":
    sys.exit(main())