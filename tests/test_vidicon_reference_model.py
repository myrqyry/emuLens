#!/usr/bin/env python3

import math
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
    return source[start:index - 1]


def excitation(luma: float, threshold: float) -> float:
    x = max(0.0, min(1.0, (luma - threshold) / max(1.0 - threshold, 0.001)))
    return x * x * (3.0 - 2.0 * x)


def decay_half_life(decay: float) -> float:
    decay = max(0.0, min(1.0, decay))
    if decay <= 0.0:
        return 0.0
    if decay <= 0.50:
        return 0.02 + (0.18 - 0.02) * (decay / 0.50)
    if decay <= 0.80:
        return 0.18 + (0.60 - 0.18) * ((decay - 0.50) / 0.30)
    if decay <= 0.90:
        return 0.60 + (1.20 - 0.60) * ((decay - 0.80) / 0.10)
    if decay <= 0.95:
        return 1.20 + (2.50 - 1.20) * ((decay - 0.90) / 0.05)
    if decay <= 0.99:
        return 2.50 + (6.00 - 2.50) * ((decay - 0.95) / 0.04)
    return 6.00 + (20.0 - 6.00) * ((decay - 0.99) / 0.01)


def frame_decay(decay: float, dt: float) -> float:
    if decay < 0.001:
        return 0.0
    if decay >= 0.9995:
        nominal_60hz = 1.0
    else:
        half_life = decay_half_life(decay)
        nominal_60hz = 2.0 ** (-(1.0 / 60.0) / half_life)
    return nominal_60hz ** (max(0.0, min(0.25, dt)) * 60.0)


def feedback_step(
    history: float,
    current: float,
    decay: float,
    threshold: float,
    nonlinear: float = 0.0,
    dt: float = 1.0 / 60.0,
) -> float:
    base = frame_decay(decay, dt)
    stored = excitation(history, threshold)
    effective = min(1.0, base + nonlinear * stored * (1.0 - base) * 0.45)
    return max(current, history * effective)


def main() -> int:
    shader = (ROOT / "data/emulens.effect").read_text()
    vidicon = function_body(shader, "PSVidicon")
    composite = function_body(shader, "PSVidiconComposite")

    required_vidicon_terms = [
        "float3 feedback_rgb = max(injected, retained)",
        "float history_luma = dot(history.rgb",
        "float stored_excitation = saturate(",
        "vid_red_lag * (1.0 - base_decay) * 1.00",
        "vid_blue_lag * (1.0 - base_decay) * 0.65",
        "vid_source_color > 0.5",
    ]
    missing = [term for term in required_vidicon_terms if term not in vidicon]
    if missing:
        print("FAIL: Vidicon full-frame feedback model is incomplete:")
        for term in missing:
            print(f"  - {term}")
        return 1

    if "lerp(feedback.rgb, registered" not in composite:
        print("FAIL: Vidicon composite must present the full feedback frame")
        return 1
    if "max(original.rgb, trail_rgb)" in composite:
        print("FAIL: sparse trail masking returned to the composite")
        return 1

    # A static source must not pump exposure: max(current, decayed history)
    # settles exactly on the current signal.
    current = 0.82
    history = 0.0
    for _ in range(240):
        history = feedback_step(history, current, 0.88, 0.70)
        if not math.isclose(history, current, rel_tol=0.0, abs_tol=1e-9):
            print("FAIL: static Vidicon source accumulates exposure")
            return 1

    # Real-footage regression: a bright object moving over a non-black
    # background must leave visible residual energy. The old sparse test used
    # a black background and failed to protect this behavior.
    history = 1.0
    background = 0.38
    residual = feedback_step(history, background, 0.94, 0.55)
    if not residual > background + 0.25:
        print("FAIL: Vidicon persistence disappears on non-black footage")
        return 1

    # Once a bright source disappears, stored charge must decay monotonically.
    history = 1.0
    samples = []
    for _ in range(8):
        history = feedback_step(history, 0.0, 0.88, 0.70)
        samples.append(history)
    if not (samples[0] > 0.0 and all(a > b for a, b in zip(samples, samples[1:]))):
        print("FAIL: Vidicon persistence does not decay monotonically after motion")
        return 1

    low = feedback_step(1.0, 0.0, 0.78, 0.70)
    high = feedback_step(1.0, 0.0, 0.975, 0.70)
    if not high > low:
        print("FAIL: higher Trail Decay no longer produces longer persistence")
        return 1

    # The artistic control should expose useful, predictable half-lives across
    # the slider rather than crushing them into 0.98-0.99.
    anchors = {
        0.50: 0.18,
        0.80: 0.60,
        0.90: 1.20,
        0.95: 2.50,
        0.99: 6.00,
    }
    for control, expected in anchors.items():
        if not math.isclose(decay_half_life(control), expected, rel_tol=0.0, abs_tol=1e-6):
            print(f"FAIL: Trail Decay {control:.2f} no longer maps to {expected:.2f}s half-life")
            return 1

    if frame_decay(1.0, 1.0 / 60.0) != 1.0:
        print("FAIL: Trail Decay 1.00 must retain history indefinitely")
        return 1

    # Time normalization means one second of decay is independent of render FPS.
    at_60 = frame_decay(0.90, 1.0 / 60.0) ** 60
    at_30 = frame_decay(0.90, 1.0 / 30.0) ** 30
    if not math.isclose(at_60, at_30, rel_tol=1e-6, abs_tol=1e-9):
        print("FAIL: Trail Decay changed with frame rate")
        return 1

    duplicate = feedback_step(0.8, 0.0, 0.95, 0.70, dt=0.0)
    if not math.isclose(duplicate, 0.8, rel_tol=0.0, abs_tol=1e-9):
        print("FAIL: duplicate same-tick render consumes Vidicon persistence")
        return 1

    print("PASS: Vidicon full-frame feedback remains visible on non-black footage")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
