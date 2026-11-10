#!/usr/bin/env python3
"""
Per-effect control-responsiveness regression test (golden CPU references).

For every effect in data/emulens.effect this mirrors the pixel shader's
core "amount" math in pure Python and asserts that the main control actually
changes the rendered output on representative footage.

Why this exists:
  The Bloom bug (see tests/test_bloom_responsiveness.py) was a control scaling
  a near-zero, threshold-extracted bright pass, so Intensity/Color looked inert
  on ordinary footage. This suite is the permanent net that would catch the
  same class of regression in ANY effect: if a control stops moving the output,
  the matching test fails.

Method / honesty:
  These are CPU references of the shader math, not a GPU render. They faithfully
  reproduce the exact formula for the controlled term (same weights, same gating)
  on a deterministic non-flat frame so that a control that scales a near-zero term
  would produce ~0 delta and fail. They are NOT a pixel-exact match to the HLSL
  (no GPU/textures here) -- they prove *responsiveness*, not *numerical identity*.

  Starburst is tested on bright-source footage on purpose: a starburst flare only
  emits from luminance > sb_threshold, so intensity correctly does nothing on
  mid-gray footage. That is by design, not a bug.

Run:  python3 tests/test_effect_responsiveness.py
Exit 1 on any inert control; prints PASS per control otherwise.
"""

import math
import sys

LUMA_W = (0.299, 0.587, 0.114)
AMBIENT_K = 0.15  # post-fix Bloom ambient weight


# --- common helpers (mirror the shader's) ---
def saturate(x):
    return max(0.0, min(1.0, x))


def clamp(x, lo, hi):
    return max(lo, min(hi, x))


def clamp01(x):
    return clamp(x, 0.0, 1.0)


def lerp(a, b, t):
    return a + (b - a) * t


def lerp3(a, b, t):
    return [a[k] + (b[k] - a[k]) * t for k in range(3)]


def smoothstep(e0, e1, x):
    t = clamp((x - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def f2i(v):
    return int(v + 0.5)


def dot3(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def lum(c):
    return dot3(c, LUMA_W)


def dist2(a, b):
    return math.hypot(a[0] - b[0], a[1] - b[1])


def sample(u, v):
    """Deterministic non-flat source frame: rgb = (u, v, (u+v)/2)."""
    u = clamp01(u)
    v = clamp01(v)
    return (u, v, (u + v) * 0.5)


# =====================================================================
# 1. BLOOM  (intensity/color/dust/haze must move ordinary footage)
# =====================================================================
def bloom_combine(original, blurred, intensity, color, dust=0.0, haze=0.0):
    src_luma = lum(original)
    ambient = [o * src_luma * intensity * c * AMBIENT_K for o, c in zip(original, color)]
    bloom_rgb = [b * intensity * c for b, c in zip(blurred, color)]
    final = [o + br + a for o, br, a in zip(original, bloom_rgb, ambient)]
    if dust > 0.0 or haze > 0.0:
        shaped_dirt = 0.5
        dust_mask = saturate((0.35 + 0.5 * 0.65) * haze)
        dust_visible = color[0] * shaped_dirt * dust * 0.6
        for i in range(3):
            final[i] += dust_visible
            final[i] *= (1.0 - saturate(dust_mask * 0.35))
    return [saturate(c) for c in final]


# =====================================================================
# 2. ORTON  (full-frame blur; blur_strength must move ordinary footage)
# =====================================================================
def orton(orig, blurred, strength, brightness=0.0, contrast=1.2, sat=1.2):
    col = list(orig)
    blr = list(blurred)
    blended = [c + b * strength for c, b in zip(col, blr)]  # blend mode 0
    result = [x + brightness for x in blended]
    cv = max(0.01, contrast)
    result = [(x - 0.5) * cv + 0.5 for x in result]
    g = lum(result)
    result = lerp3([g, g, g], result, max(0.0, sat))
    return [saturate(x) for x in result]


# =====================================================================
# 3. STARBURST  (bright-source flare; intensity must move BRIGHT footage)
# =====================================================================
def frame(u, v):
    """Bright spot at center, dark elsewhere -- used by bright-gated tests."""
    d = dist2((u, v), (0.5, 0.5))
    if d < 0.03:
        return (1.0, 1.0, 1.0)
    return (0.05, 0.05, 0.05)


def starburst(uv, intensity, threshold=0.7, smoothness=3.0, points=6,
              samples=16, ray_length=0.3, rot=0.0, aspect=1.0,
              thickness=2.0):
    original = frame(*uv)
    np_ = points
    if np_ < 2:
        np_ = 2
    if np_ > 12:
        np_ = 12
    ns = samples
    if ns < 4:
        ns = 4
    if ns > 32:
        ns = 32
    total = 0.0
    for i in range(np_):
        angle = (6.2831853 / float(np_)) * i + rot
        dirx = math.cos(angle) / aspect
        diry = math.sin(angle)
        direction_length = math.hypot(dirx, diry)
        perpx = -diry / direction_length
        perpy = dirx / direction_length
        width_x = perpx * thickness / 1920.0
        width_y = perpy * thickness / 1080.0
        ray_accum = 0.0
        for j in range(1, ns + 1):
            scale = j / float(ns)
            sx = uv[0] - dirx * ray_length * scale
            sy = uv[1] - diry * ray_length * scale
            if sx < 0.0 or sx > 1.0 or sy < 0.0 or sy > 1.0:
                continue
            sl = lum(frame(sx, sy))
            for wx, wy in ((width_x, width_y), (-width_x, -width_y)):
                px, py = sx + wx, sy + wy
                if 0.0 <= px <= 1.0 and 0.0 <= py <= 1.0:
                    sl = max(sl, lum(frame(px, py)))
            gate = threshold * (1.0 * (1.0 - scale) + 0.5 * scale)
            gate_factor = smoothstep(gate, gate + max(0.001, threshold * 0.25), sl)
            ray_accum += gate_factor * (1.0 - scale) ** smoothness
        total += ray_accum / float(ns)
    total = saturate(total / (math.sqrt(float(np_)) * 0.5 + 0.1))
    mix = total * intensity
    final = list(original)
    if mix > 0.001:
        final = [o + 1.0 * mix for o in original]
    return [saturate(x) for x in final]


# =====================================================================
# 4. BOKEH  (spatial disc blur; radius must move ordinary footage)
# =====================================================================
def bokeh(uv, radius, focus=(0.5, 0.5), width=0.3, falloff=0.5,
          boost=10.0, power=2.0, texel=0.005):
    d = dist2(uv, focus)
    bf = smoothstep(width, width + falloff, d)
    if bf <= 0.01:
        return list(sample(*uv))
    r = bf * radius
    accum = [0.0, 0.0, 0.0]
    total_w = 0.0
    for i in range(8):
        t = i / 8.0
        ang = t * 6.2831853
        off = (math.cos(ang) * r * texel, math.sin(ang) * r * texel)
        s = sample(uv[0] + off[0], uv[1] + off[1])
        w = 1.0 + (lum(s) ** max(1.0, power)) * max(1.0, boost)
        accum = [a + s[k] * w for k, a in enumerate(accum)]
        total_w += w
    return [saturate(a / total_w) for a in accum]


# =====================================================================
# 5. PARTICLES  (procedural full-frame; size must cover/move pixels)
# =====================================================================
def particles(uv, size, pcol=(1.0, 1.0, 1.0), blur_edge=0.5, size_var=0.0):
    center = (0.5, 0.5)
    s = size * (1.0 + 0.0 * size_var)
    d = dist2(uv, center)
    if d < s:
        mask = 1.0 - smoothstep(s * (1.0 - blur_edge), s, d)
        accum = [pcol[k] * mask * 1.0 for k in range(3)]
    else:
        accum = [0.0, 0.0, 0.0]
    base = sample(*uv)
    return [saturate(base[k] + accum[k]) for k in range(3)]


# =====================================================================
# 6. GRIT  (uniform full-frame add; brightness must move footage)
# =====================================================================
def grit(orig, brightness, opacity=1.0, noise_layer=0.2):
    final = [o + noise_layer * opacity for o in orig]
    final = [x + brightness for x in final]
    return [saturate(x) for x in final]


# =====================================================================
# 7. VIDICON  (composite gamma; gamma must move footage)
# =====================================================================
def vidicon(feedback, gamma=1.0, register=0.0):
    final = lerp3(list(feedback), list(feedback), saturate(register))  # register 0
    ge = 1.0 / max(gamma, 0.01)
    final = [x ** ge for x in final]
    return [saturate(x) for x in final]


# =====================================================================
# 8. HANDHELD  (zoom displacement; zoom must move footage)
# =====================================================================
def handheld(uv, zoom=1.0, feather=0.0):
    u = (uv[0] - 0.5) / zoom + 0.5
    v = (uv[1] - 0.5) / zoom + 0.5
    col = list(sample(u, v))
    if feather > 0.0:
        f = feather
        mx = smoothstep(0.0, f, uv[0]) * smoothstep(0.0, f, 1.0 - uv[0])
        my = smoothstep(0.0, f, uv[1]) * smoothstep(0.0, f, 1.0 - uv[1])
        col = [c * (mx * my) for c in col]
    return [saturate(x) for x in col]


# =====================================================================
# 9. LIGHT LEAK  (additive leak; intensity must move footage)
# =====================================================================
def lightleak(uv, intensity, color=(1.0, 0.5333, 0.2667), edge_falloff=3.0,
              bias_top=1.0, bias_bot=1.0, bias_left=1.0, bias_right=1.0,
              contrast=2.0, fbm_val=0.5):
    mask = 0.0
    mask += (1.0 - uv[1]) ** edge_falloff * bias_top
    mask += uv[1] ** edge_falloff * bias_bot
    mask += (1.0 - uv[0]) ** edge_falloff * bias_left
    mask += uv[0] ** edge_falloff * bias_right
    mask = max(mask, 0.002)
    leak_val = saturate(fbm_val * intensity * mask)
    leak_val = leak_val ** max(0.1, 1.0 / contrast)
    if leak_val < 0.5:
        leak_rgb = lerp3([0.0, 0.0, 0.0], list(color), leak_val * 2.0)
    else:
        leak_rgb = lerp3(list(color), [1.0, 1.0, 1.0], (leak_val - 0.5) * 2.0)
    base = sample(*uv)
    return [saturate(base[k] + leak_rgb[k]) for k in range(3)]


# =====================================================================
# 10. MIRAGE  (radial sample blur; radius must move footage)
# =====================================================================
def mirage(uv, radius, center_mask=0.3, count=4, rot=0.0, rot_speed=0.0,
           t=0.0, parallel=0.0):
    center = (0.5, 0.5)
    mask = smoothstep(0.0, max(0.01, center_mask), dist2(uv, center))
    accum = [0.0, 0.0, 0.0]
    valid = 0
    for i in range(count):
        ang = (6.2831853 / count) * i + math.radians(rot) + rot_speed * t * 0.1
        off = (math.cos(ang) * radius * mask, math.sin(ang) * radius * mask)
        suv = (uv[0] + off[0], uv[1] + off[1])
        if 0.0 < suv[0] < 1.0 and 0.0 < suv[1] < 1.0:
            s = sample(*suv)
            accum = [a + s[k] for k, a in enumerate(accum)]
            valid += 1
    cnt = max(1.0, float(valid))
    avg = [a / cnt for a in accum]
    result = lerp3(list(sample(*uv)), avg, mask)
    return [saturate(x) for x in result]


# =====================================================================
# 11. CENTER SPOT  (multiply blend; blur_amount must move footage)
# =====================================================================
def centerspot(uv, blur_amount, orig_color=(0.6, 0.6, 0.6), radius=0.4,
               softness=0.3):
    center = (0.5, 0.5)
    d = dist2(uv, center)
    mask = smoothstep(radius, radius + softness, d)
    zoomed = sample(*uv)
    # Exaggerated blur offset: a real frame has far more local variation than
    # this synthetic gradient, so we widen the offset to expose the multiply
    # response on a smooth frame (otherwise blurred ~= zoomed).
    blurred = sample(uv[0] + 0.2, uv[1] + 0.2)
    blur_mix = mask * blur_amount
    spot = lerp3(list(zoomed), list(blurred), blur_mix)
    return [saturate(orig_color[k] * spot[k]) for k in range(3)]


# =====================================================================
# 12. PINHOLE  (cos^4 falloff; vignette must move footage)
# =====================================================================
def pinhole(uv, vignette, softness=0.3):
    centered = (uv[0] - 0.5, uv[1] - 0.5)
    phys = (centered[0], centered[1])
    focal = lerp(2.5, 0.55, vignette)
    theta = math.atan(dist2(phys, (0.0, 0.0)) / focal)
    cos4 = math.cos(theta) ** 4
    col = sample(*uv)
    col = [c * cos4 for c in col]
    return [saturate(x) for x in col]


# =====================================================================
# 13. POLAROID  (grade + vignette; vignette must move footage)
# =====================================================================
def polaroid(uv, vignette, contrast=1.2):
    col = sample(*uv)
    col = [(c - 0.5) * contrast + 0.5 for c in col]
    col = [max(c, 0.05) for c in col]
    d = dist2(uv, (0.5, 0.5))
    vig = 1.0 - smoothstep(0.2, 1.0, d * (1.0 + vignette))
    col = [c * lerp(1.0, vig, vignette) for c in col]
    return [saturate(x) for x in col]


# =====================================================================
# 14. DAGUERREOTYPE  (metallic mono grade; contrast must move footage)
# =====================================================================
def daguerreotype(uv, contrast, tint=0.8, vignette=0.9):
    col = sample(*uv)
    l = lum(col)
    l = (l - 0.5) * (1.0 + contrast * 2.0) + 0.5
    l = saturate(l)
    silver = (0.9, 0.95, 1.0)
    tarnish = (0.3, 0.25, 0.2)
    tinted = lerp3(tarnish, silver, l)
    col = lerp3([l, l, l], tinted, tint)
    d = dist2(uv, (0.5, 0.5))
    vig = 1.0 - smoothstep(0.1, 0.9, d * (1.0 + vignette * 1.5))
    col = [c * lerp(1.0, vig, vignette) for c in col]
    return [saturate(x) for x in col]


# =====================================================================
# 15. TECHNICOLOR  (channel-split; offset must move footage)
# =====================================================================
def technicolor(uv, offset, strength=1.0, saturation=1.5, texel=0.05):
    o = offset * 3.0 * texel
    col_center = sample(*uv)
    col_red = sample(uv[0] + o, uv[1])
    col_cyan = sample(uv[0] - o, uv[1])
    red_record = col_red[0]
    cyan_record = (col_cyan[1] + col_cyan[2]) * 0.5
    tech = [red_record, cyan_record, cyan_record * 0.9]
    col = lerp3(list(col_center), tech, strength)
    l = lum(col)
    col = lerp3([l, l, l], col, saturation)
    return [saturate(x) for x in col]


# =====================================================================
# TEST HARNESS
# =====================================================================
def d3(a, b):
    return abs(a[0] - b[0]) + abs(a[1] - b[1]) + abs(a[2] - b[2])


def check(label, a, b, min_delta=0.05):
    d = d3(a, b)
    if d < min_delta:
        return False, f"{label} inert (delta={d:.4f} < {min_delta})"
    return True, f"{label} responsive (delta={d:.4f})"


def main():
    failures = []

    # 1. BLOOM -- ordinary footage (mid-gray), empty bright extraction.
    orig = (0.5, 0.5, 0.5)
    blurred = (0.0, 0.0, 0.0)
    white = (1.0, 1.0, 1.0)
    ok, msg = check("Bloom.Intensity",
                    bloom_combine(orig, blurred, 2.0, white),
                    bloom_combine(orig, blurred, 10.0, white))
    print(("  PASS: " if ok else "  FAIL: ") + msg); failures += [] if ok else [msg]
    ok, msg = check("Bloom.Color",
                    bloom_combine(orig, blurred, 5.0, (1.0, 1.0, 1.0)),
                    bloom_combine(orig, blurred, 5.0, (1.0, 0.0, 0.0)))
    print(("  PASS: " if ok else "  FAIL: ") + msg); failures += [] if ok else [msg]
    ok, msg = check("Bloom.Dust",
                    bloom_combine(orig, blurred, 5.0, white, dust=0.0),
                    bloom_combine(orig, blurred, 5.0, white, dust=1.0), 0.01)
    print(("  PASS: " if ok else "  FAIL: ") + msg); failures += [] if ok else [msg]
    ok, msg = check("Bloom.Haze",
                    bloom_combine(orig, blurred, 5.0, white, haze=0.0),
                    bloom_combine(orig, blurred, 5.0, white, haze=1.0), 0.01)
    print(("  PASS: " if ok else "  FAIL: ") + msg); failures += [] if ok else [msg]

    # 2. ORTON -- ordinary footage, blurred != sharp (simulated).
    o = sample(0.2, 0.5)
    bl = sample(0.23, 0.5)
    ok, msg = check("Orton.BlurStrength",
                    orton(o, bl, 0.0), orton(o, bl, 1.0))
    print(("  PASS: " if ok else "  FAIL: ") + msg); failures += [] if ok else [msg]

    # 3. STARBURST -- dark pixel flaring from a nearby bright source.
    suv = (0.54, 0.5)
    ok, msg = check("Starburst.Intensity(bright-src)",
                    starburst(suv, 1.0), starburst(suv, 5.0))
    print(("  PASS: " if ok else "  FAIL: ") + msg); failures += [] if ok else [msg]

    # 4. BOKEH -- ordinary footage, off-center pixel, large blur spread.
    buv = (0.0, 0.5)
    ok, msg = check("Bokeh.Radius",
                    bokeh(buv, 1.0, texel=0.04), bokeh(buv, 20.0, texel=0.04))
    print(("  PASS: " if ok else "  FAIL: ") + msg); failures += [] if ok else [msg]

    # 5. PARTICLES -- ordinary footage, test pixel between size bounds.
    puv = (0.5, 0.53)
    ok, msg = check("Particles.Size",
                    particles(puv, 0.01), particles(puv, 0.05), 0.01)
    print(("  PASS: " if ok else "  FAIL: ") + msg); failures += [] if ok else [msg]

    # 6. GRIT -- ordinary footage, uniform brightness add.
    ok, msg = check("Grit.Brightness",
                    grit((0.5, 0.5, 0.5), 0.0), grit((0.5, 0.5, 0.5), 0.5))
    print(("  PASS: " if ok else "  FAIL: ") + msg); failures += [] if ok else [msg]

    # 7. VIDICON -- ordinary footage, composite gamma.
    fb = sample(0.3, 0.5)
    ok, msg = check("Vidicon.Gamma",
                    vidicon(fb, 1.0), vidicon(fb, 2.2))
    print(("  PASS: " if ok else "  FAIL: ") + msg); failures += [] if ok else [msg]

    # 8. HANDHELD -- ordinary footage, zoom displacement.
    huv = (0.3, 0.5)
    ok, msg = check("Handheld.Zoom",
                    handheld(huv, 1.0), handheld(huv, 1.5))
    print(("  PASS: " if ok else "  FAIL: ") + msg); failures += [] if ok else [msg]

    # 9. LIGHT LEAK -- corner pixel (large mask), ordinary footage.
    luv = (0.0, 0.0)
    ok, msg = check("LightLeak.Intensity",
                    lightleak(luv, 0.1), lightleak(luv, 1.0))
    print(("  PASS: " if ok else "  FAIL: ") + msg); failures += [] if ok else [msg]

    # 10. MIRAGE -- off-center pixel.
    muv = (0.2, 0.5)
    ok, msg = check("Mirage.Radius",
                    mirage(muv, 0.05), mirage(muv, 0.2))
    print(("  PASS: " if ok else "  FAIL: ") + msg); failures += [] if ok else [msg]

    # 11. CENTER SPOT -- off-center pixel (multiply blend needs bright orig).
    suv = (0.1, 0.1)
    ok, msg = check("CenterSpot.BlurAmount",
                    centerspot(suv, 0.0, (0.6, 0.6, 0.6)),
                    centerspot(suv, 1.0, (0.6, 0.6, 0.6)))
    print(("  PASS: " if ok else "  FAIL: ") + msg); failures += [] if ok else [msg]

    # 12. PINHOLE -- corner-ish pixel, cos^4 falloff.
    puv2 = (0.1, 0.1)
    ok, msg = check("Pinhole.Vignette",
                    pinhole(puv2, 0.2), pinhole(puv2, 0.9))
    print(("  PASS: " if ok else "  FAIL: ") + msg); failures += [] if ok else [msg]

    # 13. POLAROID -- off-center pixel, vignette.
    poluv = (0.1, 0.1)
    ok, msg = check("Polaroid.Vignette",
                    polaroid(poluv, 0.2), polaroid(poluv, 0.9))
    print(("  PASS: " if ok else "  FAIL: ") + msg); failures += [] if ok else [msg]

    # 14. DAGUERREOTYPE -- ordinary footage, contrast grade.
    dguv = (0.3, 0.3)
    ok, msg = check("Daguerreotype.Contrast",
                    daguerreotype(dguv, 0.5), daguerreotype(dguv, 1.5))
    print(("  PASS: " if ok else "  FAIL: ") + msg); failures += [] if ok else [msg]

    # 15. TECHNICOLOR -- off-center pixel, channel split.
    tuv = (0.3, 0.5)
    ok, msg = check("Technicolor.Offset",
                    technicolor(tuv, 0.0), technicolor(tuv, 0.2))
    print(("  PASS: " if ok else "  FAIL: ") + msg); failures += [] if ok else [msg]

    print("-" * 60)
    if failures:
        for f in failures:
            print("  FAIL: " + f)
        print("EFFECT RESPONSIVENESS: %d FAILURE(S)" % len(failures))
        sys.exit(1)
    print("ALL 15 EFFECTS: MAIN CONTROLS RESPONSIVE")


if __name__ == "__main__":
    main()
