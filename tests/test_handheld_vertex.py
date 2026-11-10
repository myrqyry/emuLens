#!/usr/bin/env python3

import sys
from pathlib import Path


CPU_TRANSFORM_UNIFORMS = [
    "hand_pos_offset",
    "hand_rot_rad",
    "hand_zoom_factor",
]


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    source = (root / "src/strategies/strategy-handheld.c").read_text(
        encoding="utf-8"
    )
    params = (root / "src/lens-params.h").read_text(encoding="utf-8")
    effect = (root / "data/emulens.effect").read_text(encoding="utf-8")

    for uniform in CPU_TRANSFORM_UNIFORMS:
        if uniform not in effect:
            print(f"FAIL: missing handheld CPU transform uniform {uniform}")
            return 1
        if uniform not in source and uniform not in params:
            print(f"FAIL: strategy does not bind {uniform}")
            return 1

    if "VSHandheld" in effect:
        print("FAIL: handheld still uses a custom vertex shader")
        return 1

    vertical_flip_patterns = [
        "uv.y = 1.0 - uv.y",
        "uv = float2(uv.x, 1.0 - uv.y)",
        "uv.y *= -1.0",
        "float2(uv.x, -uv.y)",
    ]
    if any(pattern in effect for pattern in vertical_flip_patterns):
        print("FAIL: handheld effect flips the image vertically")
        return 1

    handheld_start = effect.find("float4 PSHandheld(")
    handheld_end = effect.find("technique DrawHandheld")
    if handheld_start == -1 or handheld_end == -1:
        print("FAIL: could not locate handheld pixel shader")
        return 1

    ps_handheld = effect[handheld_start:handheld_end]
    for uniform in CPU_TRANSFORM_UNIFORMS:
        if uniform not in ps_handheld:
            print(f"FAIL: handheld pixel shader does not use {uniform}")
            return 1

    if "motion_initialized" not in source:
        print("FAIL: handheld lacks persistent motion state")
        return 1

    if "motion_velocity" not in source:
        print("FAIL: handheld blur is not velocity-linked")
        return 1

    print("PASS: handheld transform is CPU-driven and velocity-aware")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
