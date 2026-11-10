#!/usr/bin/env python3
"""CPU contract for Bloom / Dusty Lens causal behavior."""

import math
import sys

LUMA_W = (0.299, 0.587, 0.114)
AMBIENT_K = 0.15


def saturate(x):
    return max(0.0, min(1.0, x))


def smoothstep(lo, hi, x):
    t = saturate((x - lo) / max(hi - lo, 1e-9))
    return t * t * (3.0 - 2.0 * t)


def combine(original, blurred, intensity, color, dust_amount=0.0,
            haze_amount=0.0, dust_specular=0.0, shaped_dirt=0.78):
    src_luma = sum(o * w for o, w in zip(original, LUMA_W))
    ambient = [
        o * src_luma * intensity * c * AMBIENT_K
        for o, c in zip(original, color)
    ]
    bloom_rgb = [b * intensity * c for b, c in zip(blurred, color)]
    final = [o + br + a for o, br, a in zip(original, bloom_rgb, ambient)]

    highlight_luma = sum(b * w for b, w in zip(blurred, LUMA_W))
    light_energy = max(0.0, highlight_luma * intensity)
    light_gate = saturate(light_energy * 2.0)

    dirt = shaped_dirt
    dust_presence = smoothstep(0.24, 0.92, shaped_dirt)
    dust_scatter = dust_presence * dust_amount * light_gate
    fine_dirt = smoothstep(0.68, 0.96, shaped_dirt)
    glint = fine_dirt * light_gate * light_gate * dust_specular * dust_amount

    for i in range(3):
        final[i] += bloom_rgb[i] * dust_scatter * 1.10
        final[i] += color[i] * saturate(light_energy) * glint * 0.35

    attenuation = 1.0 - saturate(
        dust_presence * dust_amount * light_gate * 0.10
    )
    final = [v * attenuation for v in final]

    haze_mask = saturate(haze_amount * light_gate * (0.30 + dirt * 0.70))
    for i in range(3):
        final[i] += bloom_rgb[i] * haze_mask * 0.90
        final[i] += color[i] * saturate(light_energy) * haze_mask * 0.12

    return [saturate(c) for c in final]


def delta(a, b):
    return sum(abs(x - y) for x, y in zip(a, b))


def main():
    original = (0.5, 0.5, 0.5)
    empty_bright_pass = (0.0, 0.0, 0.0)
    lit_bright_pass = (0.30, 0.28, 0.24)
    white = (1.0, 1.0, 1.0)
    failures = []

    # Clean bloom controls remain useful even without a clipped highlight.
    lo = combine(original, empty_bright_pass, 2.0, white)
    hi = combine(original, empty_bright_pass, 10.0, white)
    if delta(lo, hi) < 0.05:
        failures.append("Bloom Intensity became inert on ordinary footage")

    tint = combine(original, empty_bright_pass, 5.0, (1.0, 0.0, 0.0))
    if delta(combine(original, empty_bright_pass, 5.0, white), tint) < 0.05:
        failures.append("Bloom Color became inert")

    # No captured light: lens contamination should mostly disappear.
    clean_dark = combine(original, empty_bright_pass, 0.8, white)
    dusty_dark = combine(
        original, empty_bright_pass, 0.8, white,
        dust_amount=0.8, haze_amount=0.6, dust_specular=1.0
    )
    if delta(clean_dark, dusty_dark) > 1e-9:
        failures.append("Dust/haze still behaves like a source-independent overlay")

    # Bright-pass energy: the same contamination must become visible.
    clean_lit = combine(original, lit_bright_pass, 0.8, white)
    dusty_lit = combine(
        original, lit_bright_pass, 0.8, white,
        dust_amount=0.8, haze_amount=0.6, dust_specular=1.0
    )
    if delta(clean_lit, dusty_lit) < 0.03:
        failures.append("Lens contamination does not react visibly to captured light")

    # More bright-pass energy should reveal more contamination.
    weak = combine(
        original, (0.06, 0.06, 0.06), 0.8, white,
        dust_amount=0.8, haze_amount=0.6, dust_specular=1.0
    )
    strong = combine(
        original, (0.40, 0.40, 0.40), 0.8, white,
        dust_amount=0.8, haze_amount=0.6, dust_specular=1.0
    )
    clean_weak = combine(original, (0.06, 0.06, 0.06), 0.8, white)
    clean_strong = combine(original, (0.40, 0.40, 0.40), 0.8, white)
    weak_response = delta(weak, clean_weak)
    strong_response = delta(strong, clean_strong)
    if not strong_response > weak_response:
        failures.append("Dust response no longer increases with incident light")

    if failures:
        for failure in failures:
            print("FAIL:", failure)
        return 1

    print("PASS: Bloom dirt/haze stays dormant without light and responds to highlights")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
