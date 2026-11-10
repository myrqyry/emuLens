#!/usr/bin/env python3
"""
Render-contract and locale validation tests for emuLensFX.

Verifies:
1. Every obs_module_text("Key") in C has a matching key in en-US.ini.
2. Every P_* shader param referenced in C resolves to a string that
   exists in data/emulens.effect.
3. Every technique name cached in C exists in data/emulens.effect.
4. Strategy render functions do not fetch cached settings on the render path.
5. apply_gaussian_blur does not early-return cached frame output.
6. Vidicon and Bloom multipass effect paths refresh dynamic outputs.
"""

import os
import re
import sys

REPO = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
LOCALE_PATH = os.path.join(REPO, "data", "locale", "en-US.ini")
EFFECT_PATH = os.path.join(REPO, "data", "emulens.effect")
PARAMS_PATH = os.path.join(REPO, "src", "lens-params.h")

errors = []


def read_file(path):
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


# ----------------------------------------------------------------
# Parse P_* macro definitions from lens-params.h
# ----------------------------------------------------------------
def parse_param_macros():
    text = read_file(PARAMS_PATH)
    macros = {}
    for m in re.finditer(r'#define\s+(P_\w+)\s+"([^"]+)"', text):
        macros[m.group(1)] = m.group(2)
    return macros


# ----------------------------------------------------------------
# Extract function body from C source given function name
# ----------------------------------------------------------------
def extract_function_body(text, func_name):
    """Return the content of the function body for func_name, or empty string."""
    pattern = r'\b(' + re.escape(func_name) + r')\s*\([^)]*\)\s*\{'
    m = re.search(pattern, text)
    if not m:
        return ""
    start = m.end()
    depth = 1
    i = start
    while i < len(text) and depth > 0:
        if text[i] == '{':
            depth += 1
        elif text[i] == '}':
            depth -= 1
        i += 1
    if depth == 0:
        return text[start:i - 1]
    return ""


# ----------------------------------------------------------------
# 1. Locale key coverage
# ----------------------------------------------------------------
def check_locale_keys():
    locale_text = read_file(LOCALE_PATH)
    locale_keys = set()
    for line in locale_text.splitlines():
        m = re.match(r'^([a-zA-Z0-9_:/ .-]+)=', line)
        if m:
            locale_keys.add(m.group(1))

    c_source_dirs = [
        os.path.join(REPO, "src"),
        os.path.join(REPO, "src", "strategies"),
    ]
    c_files = []
    for d in c_source_dirs:
        if os.path.isdir(d):
            c_files.extend(os.path.join(d, f) for f in os.listdir(d)
                           if f.endswith((".c", ".h")))

    used_keys = set()
    key_re = re.compile(r'obs_module_text\("([^"]+)"\)')
    for path in c_files:
        content = read_file(path)
        for m in key_re.finditer(content):
            used_keys.add(m.group(1))

    missing = sorted(used_keys - locale_keys)
    if missing:
        errors.append(
            f"Locale keys missing from en-US.ini ({len(missing)}):\n  " +
            "\n  ".join(missing)
        )
    else:
        print(f"  PASS: All {len(used_keys)} obs_module_text keys found in locale")


# ----------------------------------------------------------------
# 2. Shader param coverage (resolves P_* macros)
# ----------------------------------------------------------------
def check_shader_params():
    effect_text = read_file(EFFECT_PATH)
    effect_params = set()
    for m in re.finditer(r'^uniform\s+\S+\s+(\w+)\s*(?:=|;)', effect_text, re.MULTILINE):
        effect_params.add(m.group(1))

    param_macros = parse_param_macros()

    c_source_dirs = [
        os.path.join(REPO, "src"),
        os.path.join(REPO, "src", "strategies"),
    ]
    c_files = []
    for d in c_source_dirs:
        if os.path.isdir(d):
            c_files.extend(os.path.join(d, f) for f in os.listdir(d)
                           if f.endswith((".c", ".h")))

    all_text = "\n".join(read_file(p) for p in c_files)

    # Collect literal strings from GET_PARAM_SAFE and gs_effect_get_param_by_name
    c_shader_params = set()
    getparam_re = re.compile(r'GET_PARAM_SAFE\([^,]+,\s*\w+,\s*"([^"]+)"\)')
    gseffect_re = re.compile(r'gs_effect_get_param_by_name\([^,]+,\s*"([^"]+)"\)')
    for m in getparam_re.finditer(all_text):
        c_shader_params.add(m.group(1))
    for m in gseffect_re.finditer(all_text):
        c_shader_params.add(m.group(1))

    # Collect P_* references from GET_PARAM_SAFE and resolve via macros
    p_ref_re = re.compile(r'GET_PARAM_SAFE\([^,]+,\s*\w+,\s*(P_\w+)\)')
    for m in p_ref_re.finditer(all_text):
        macro_name = m.group(1)
        if macro_name in param_macros:
            c_shader_params.add(param_macros[macro_name])
        else:
            errors.append(f"P_* macro '{macro_name}' used but not defined in lens-params.h")

    # Also check SET_PARAM_* calls for P_* references
    setparam_p_re = re.compile(r'SET_PARAM_\w+\([^,]+,\s*([^)]+)\)')
    setparam_matches = set()
    for m in setparam_p_re.finditer(all_text):
        arg = m.group(1).strip()
        if arg.startswith("P_"):
            setparam_matches.add(arg)
    for macro_name in sorted(setparam_matches):
        if macro_name in param_macros:
            c_shader_params.add(param_macros[macro_name])
        else:
            errors.append(f"SET_PARAM uses unknown macro '{macro_name}'")

    missing = sorted(c_shader_params - effect_params)
    if missing:
        errors.append(
            f"Shader params referenced in C but missing from emulens.effect ({len(missing)}):\n  " +
            "\n  ".join(missing)
        )
    else:
        print(f"  PASS: All {len(c_shader_params)} shader params found in effect file")


# ----------------------------------------------------------------
# 3. Technique name coverage
# ----------------------------------------------------------------
def check_techniques():
    effect_text = read_file(EFFECT_PATH)
    effect_techs = set()
    for m in re.finditer(r'^technique\s+(\w+)\s*\{', effect_text, re.MULTILINE):
        effect_techs.add(m.group(1))

    core_path = os.path.join(REPO, "src", "lens-core.c")
    core_text = read_file(core_path)
    c_techs = set()
    for m in re.finditer(r'GET_TECH_SAFE\([^,]+,\s*[^,]+\s*,\s*"([^"]+)"\s*\)', core_text):
        c_techs.add(m.group(1))

    missing = sorted(c_techs - effect_techs)
    if missing:
        errors.append(
            f"Technique names cached in C but missing from emulens.effect ({len(missing)}):\n  " +
            "\n  ".join(missing)
        )
    else:
        print(f"  PASS: All {len(c_techs)} technique names from C found in effect file")


# ----------------------------------------------------------------
# 4. Strategy render functions -- per-function-body validation
# ----------------------------------------------------------------
def check_strategy_render_contract():
    strategy_dir = os.path.join(REPO, "src", "strategies")
    core_text = read_file(os.path.join(REPO, "src", "lens-core.c"))
    lens_update_body = extract_function_body(core_text, "lens_update")

    if not lens_update_body:
        errors.append("Could not find lens_update body")
        return

    if "strat->update_settings" in lens_update_body:
        errors.append("lens_update dispatches shader param updates outside render")

    broken = []
    for fname in sorted(os.listdir(strategy_dir)):
        if not fname.startswith("strategy-") or not fname.endswith(".c"):
            continue
        prefix = fname[len("strategy-"):-len(".c")].replace("-", "_")
        path = os.path.join(strategy_dir, fname)
        text = read_file(path)

        render_func = prefix + "_render"
        update_func = prefix + "_update"
        body = extract_function_body(text, render_func)

        if not body:
            broken.append(f"{fname}: could not find '{render_func}' body")
            continue

        if "lens_acquire_cached_settings" in body or "cached_settings" in body:
            broken.append(f"{fname}: '{render_func}' body fetches cached settings")
            continue

        if update_func in body:
            broken.append(f"{fname}: '{render_func}' body still calls '{update_func}'")

    if broken:
        errors.append(
            f"Strategy render function contract violations:\n  " +
            "\n  ".join(broken)
        )
    else:
        print("  PASS: All strategy render functions avoid render-path settings fetches")


# ----------------------------------------------------------------
# 5. Blur cache does not early-return stale frame
# ----------------------------------------------------------------
def check_blur_cache():
    core_path = os.path.join(REPO, "src", "lens-core.c")
    text = read_file(core_path)

    # Bad pattern: "if (!changed && dims_match && filter->last_valid_blur_result) return"
    if re.search(r'if\s*\(!\s*changed\s*&&\s*dims_match\s*&&', text):
        errors.append(
            "apply_gaussian_blur still returns last_valid_blur_result as a cache hit! "
            "It should only be used as an error fallback."
        )
    else:
        print("  PASS: Blur cache does not early-return stale frame data")

    # Verify lens_render does NOT clear last_valid_blur_result — only
    # apply_gaussian_blur and recreate_texrenders_if_needed should manage it.
    render_body = extract_function_body(text, "lens_render")
    if "last_valid_blur_result = NULL" in render_body:
        errors.append(
            "lens_render unconditionally clears last_valid_blur_result before "
            "any blur pass. Let apply_gaussian_blur manage fallback lifecycle."
        )
    else:
        print("  PASS: lens_render does not clear blur fallback")


# ----------------------------------------------------------------
# 6. Vidicon feedback composite refreshes every frame
# ----------------------------------------------------------------
def check_vidicon_output_reset():
    path = os.path.join(REPO, "src", "strategies", "strategy-vidicon.c")
    text = read_file(path)
    body = extract_function_body(text, "vidicon_render")

    reset_pos = body.find("gs_texrender_reset(filter->output_texrender)")
    begin_pos = body.find("gs_texrender_begin(filter->output_texrender")
    if reset_pos < 0 or begin_pos < 0 or reset_pos > begin_pos:
        errors.append(
            "vidicon_render must reset output_texrender before its composite "
            "gs_texrender_begin, otherwise it can return a stale frame."
        )
    else:
        print("  PASS: Vidicon composite output texrender is reset before begin")


# ----------------------------------------------------------------
# 6b. Vidicon warmup binds history_image for Cg compilation
# ----------------------------------------------------------------
def check_vidicon_warmup_history_binding():
    path = os.path.join(REPO, "src", "lens-core.c")
    text = read_file(path)

    # The warmup block must bind a dummy texture to history_image so the
    # Vidicon technique compiles with a valid sampler (not an unbound one).
    warmup_start = text.find("warmup_rt = gs_texrender_create")
    if warmup_start < 0:
        errors.append(
            "lens-core.c warmup block not found — cannot verify Vidicon "
            "history_image binding."
        )
        return

    # Search the warmup block itself (from buffer creation through the
    # end of the technique), not a fixed char window that drifts as the
    # warmup grows.
    warmup_end = text.find("gs_texrender_end(warmup_rt)", warmup_start)
    if warmup_end < 0:
        warmup_end = warmup_start + 4000
    window = text[warmup_start:warmup_end]
    if "history_image" not in window:
        errors.append(
            "Vidicon warmup must bind a dummy texture to history_image for "
            "Cg compilation, but no history_image binding was found in the "
            "warmup block."
        )
    else:
        print("  PASS: Vidicon warmup binds history_image for compilation")


# ----------------------------------------------------------------
# 7. Particles wrap at screen edges without clipping
# ----------------------------------------------------------------
def check_particle_edge_wrap_distance():
    effect = read_file(EFFECT_PATH)
    start = effect.find("float4 PSBokehParticles")
    end = effect.find("technique DrawBokehParticles")
    body = effect[start:end] if start >= 0 and end > start else ""

    required = [
        "float2 particle_delta = abs(v_in.uv - pos)",
        "particle_delta = min(particle_delta, 1.0 - particle_delta)",
        "float dist = length(particle_delta * float2(aspect, 1.0))",
    ]
    missing = [term for term in required if term not in body]
    if missing:
        errors.append(
            "Particles use frac(pos) for wind wrapping but do not measure wrapped "
            "UV distance, so sprites clip when crossing frame edges:\n  "
            + "\n  ".join(missing)
        )
    else:
        print("  PASS: Particles use wrapped UV distance at screen edges")


# ----------------------------------------------------------------
# 8. Center Spot blur controls are wired into shader behavior
# ----------------------------------------------------------------
def check_spot_blur_controls():
    effect = read_file(EFFECT_PATH)
    start = effect.find("float4 PSCenterSpot")
    end = effect.find("technique DrawCenterSpot")
    body = effect[start:end] if start >= 0 and end > start else ""

    required = [
        "uniform float spot_blur_amount",
        "uniform float spot_blur_radius",
        "PSCenterSpotBlur(v_in.uv",
        "spot_blur_amount",
        "spot_blur_radius",
        "if (optic == 0)",
        "} else if (optic == 1) {",
    ]
    missing = [term for term in required if term not in effect and term not in body]
    if missing:
        errors.append(
            "Center Spot blur controls are missing from shader wiring:\n  "
            + "\n  ".join(missing)
        )
    else:
        print("  PASS: Center Spot blur controls drive shader blur")


# ----------------------------------------------------------------
# 9. Center Spot edge zoom uses radial streaking
# ----------------------------------------------------------------
def check_spot_radial_zoom():
    effect = read_file(EFFECT_PATH)
    start = effect.find("float4 PSCenterSpot")
    end = effect.find("technique DrawCenterSpot")
    body = effect[start:end] if start >= 0 and end > start else ""

    required = [
        "float4 PSCenterSpotRadialZoom",
        "float2 dir = uv - center",
        "for (int i = 1; i <= 6; i++)",
        "float2 samp_uv = saturate(uv + dir * amount",
        "spot_zoom_strength * mask",
        "PSCenterSpotRadialZoom(v_in.uv, center",
    ]
    missing = [term for term in required if term not in effect and term not in body]
    if missing:
        errors.append(
            "Center Spot edge zoom should model radial zoom streaking from the "
            "reference filter, but shader wiring is missing:\n  "
            + "\n  ".join(missing)
        )
    else:
        print("  PASS: Center Spot edge zoom uses radial streaking")


# ----------------------------------------------------------------
# 10. Bloom lens contamination is fixed in lens space and light-driven
# ----------------------------------------------------------------
def check_bloom_dust_path():
    effect = read_file(EFFECT_PATH)
    helper_start = effect.find("float BloomLensDirtPattern")
    combine_start = effect.find("float4 PSBloomCombine")
    combine_end = effect.find("technique DrawBloomCombine")

    helper = (
        effect[helper_start:combine_start]
        if helper_start >= 0 and combine_start > helper_start
        else ""
    )
    body = (
        effect[combine_start:combine_end]
        if combine_start >= 0 and combine_end > combine_start
        else ""
    )

    required = [
        "float BloomLensDirtPattern(float2 uv)",
        "noise_tex.Sample(noiseSampler",
        "float light_energy =",
        "float light_gate = saturate(light_energy * 2.0)",
        "float dust_scatter =",
        "final_rgb += incident_light * dust_scatter",
        "float haze_mask = saturate(",
        "float2 dirt_gradient =",
        "float refract_response =",
        "bloom_ghost_color.rgb * bloom_ghost_amount",
    ]
    missing = [
        term for term in required
        if term not in helper and term not in body
    ]
    if missing:
        errors.append(
            "Bloom lens-contamination path is missing light-driven optics:\n  "
            + "\n  ".join(missing)
        )
        return

    forbidden = [
        "dust_visible_rgb",
        "(0.6 + glint)",
        "final_rgb += bloom_color.rgb * haze_mask;",
    ]
    stale = [term for term in forbidden if term in body]
    if stale:
        errors.append(
            "Bloom lens dirt regressed to a source-independent overlay:\n  "
            + "\n  ".join(stale)
        )
        return

    if "elapsed_time" in helper:
        errors.append(
            "Bloom lens dirt is drifting over time; contamination should stay "
            "fixed to the optical surface."
        )
        return

    print("  PASS: Bloom lens dirt is fixed in lens space and revealed by light")


# ----------------------------------------------------------------
# Main
# ----------------------------------------------------------------
def main():
    print("=" * 60)
    print("emuLensFX Render-Contract Validation")
    print("=" * 60)

    check_locale_keys()
    check_shader_params()
    check_techniques()
    check_strategy_render_contract()
    check_blur_cache()
    check_vidicon_output_reset()
    check_vidicon_warmup_history_binding()
    check_particle_edge_wrap_distance()
    check_spot_blur_controls()
    check_spot_radial_zoom()
    check_bloom_dust_path()

    print("-" * 60)
    if errors:
        print(f"FAIL ({len(errors)} issue(s)):")
        for e in errors:
            print()
            print(e)
        sys.exit(1)
    else:
        print("ALL CHECKS PASSED")
        sys.exit(0)


if __name__ == "__main__":
    main()
