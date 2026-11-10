#!/usr/bin/env python3

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent


def function_body(source: str, name: str) -> str:
    match = re.search(rf"float4\s+{name}\s*\([^)]*\)\s*:\s*TARGET\s*\{{", source)
    if not match:
        raise AssertionError(f"missing shader function {name}")

    start = match.end()
    depth = 1
    index = start
    while index < len(source) and depth:
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
        index += 1

    if depth:
        raise AssertionError(f"unterminated shader function {name}")
    return source[start : index - 1]


def main() -> int:
    shader = (ROOT / "data/emulens.effect").read_text()
    ui = (ROOT / "src/lens-ui.c").read_text()
    spot = function_body(shader, "PSCenterSpot")
    pinhole = function_body(shader, "PSPinhole")

    if "P_SPOT_TINT" in ui:
        print("FAIL: Center Spot still exposes the retired no-op tint control")
        return 1

    if "P_PINHOLE_DISTORTION" in ui:
        print("FAIL: Pinhole still exposes the retired fake-distortion control")
        return 1

    if "spot_tint" in spot:
        print("FAIL: Center Spot shader still applies the tint/vignette blend")
        return 1

    if "col *= edge_mask" in pinhole:
        print("FAIL: Pinhole shader still applies a synthetic black edge mask")
        return 1

    hard_bounds = re.search(
        r"if\s*\([^)]*uv\.x\s*<\s*0\.0[^)]*\)\s*\{[^}]*return\s+float4",
        pinhole,
        re.S,
    )
    if hard_bounds:
        print("FAIL: Pinhole shader still hard-cuts out-of-bounds UVs to black")
        return 1

    required_physics_terms = [
        "float focal_length",
        "float theta = atan",
        "float cos4_falloff = pow(cos(theta), 4.0)",
        "float airy_radius_red",
        "float airy_radius_green",
        "float airy_radius_blue",
        "float3 airy_col",
    ]
    missing = [term for term in required_physics_terms if term not in pinhole]
    if missing:
        print("FAIL: Pinhole shader is missing physical pinhole optics terms:")
        for term in missing:
            print(f"  - {term}")
        return 1

    if "uv = uv * (1.0 + pinhole_distortion" in pinhole:
        print("FAIL: Pinhole shader still applies fake barrel distortion")
        return 1

    print("PASS: Spot tint removed and Pinhole uses physical falloff without synthetic black borders")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
