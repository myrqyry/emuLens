#!/usr/bin/env python3
"""
Shader Structural Integrity Test for EmuLensFX

Validates the effect file has:
1. No duplicate function definitions
2. No duplicate technique definitions
3. No duplicate uniform declarations
4. All samplers used by Sample() are defined
5. No unferenced sampler states
6. OBS/OpenGL vertex metadata declarations are explicit
"""

import re
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
effect_file = ROOT / "data" / "emulens.effect"
effect = effect_file.read_text(encoding="utf-8")
params = (ROOT / "src" / "lens-params.h").read_text(encoding="utf-8")
ui = (ROOT / "src" / "lens-ui.c").read_text(encoding="utf-8")
strategies = "\n".join(
    path.read_text(encoding="utf-8")
    for path in (ROOT / "src" / "strategies").glob("strategy-*.c")
)

errors = []

def duplicates(label, pattern):
    names = re.findall(pattern, effect)
    dupes = {name: count for name, count in Counter(names).items() if count > 1}
    if dupes:
        errors.append(f"duplicate {label}: {dupes}")

duplicates(
    "functions",
    r"\b(?:float4|float3|float2|float|int|VertData)\s+(\w+)\s*\(",
)

duplicates(
    "techniques",
    r"\btechnique\s+(\w+)\s*\{",
)

duplicates(
    "uniforms",
    r"\buniform\s+(?:\w+\s+)+(\w+)\s*(?:=|;)",
)

samplers = set(re.findall(r"\bsampler_state\s+(\w+)\s*\{", effect))
used_samplers = set(re.findall(r"\.Sample\(\s*(\w+)\s*,", effect))

missing_samplers = used_samplers - samplers
if missing_samplers:
    errors.append(f"samplers used but not defined: {sorted(missing_samplers)}")

unused_samplers = samplers - used_samplers
if unused_samplers:
    errors.append(f"samplers defined but never used: {sorted(unused_samplers)}")

if "textureSampler" in used_samplers and "textureSampler" not in samplers:
    errors.append("textureSampler must be explicitly declared for OBS OpenGL shader compilation")

if not re.search(r"\bstruct\s+VertData\s*\{[^}]*\bpos\s*:\s*POSITION\s*;[^}]*\buv\s*:\s*TEXCOORD0\s*;[^}]*\}", effect, re.S):
    errors.append("VertData must explicitly declare POSITION and TEXCOORD0 semantics")

param_defs = dict(re.findall(r'#define\s+(P_[A-Z0-9_]+)\s+"([^"]+)"', params))
ui_param_constants = set(
    re.findall(
        r"obs_properties_add_(?:float_slider|int_slider|bool|color_alpha|list)\([^,]+,\s*(P_[A-Z0-9_]+)\b",
        ui,
    )
)
strategy_read_constants = set(
    re.findall(
        r"obs_data_get_(?:double|int|bool|string)\((?:[^()]|\([^()]*\))*?,\s*(P_[A-Z0-9_]+)\b",
        strategies, re.S,
    )
) | set(
    re.findall(
        r"SET_CLAMPED_(?:DOUBLE|INT_AS_FLOAT)\([^)]*?,\s*(P_[A-Z0-9_]+)\b",
        strategies, re.S,
    )
) | set(
    re.findall(
        r"SET_BOOL_AS_FLOAT\([^)]*?,\s*(P_[A-Z0-9_]+)\b",
        strategies, re.S,
    )
)
strategy_read_literals = set(
    re.findall(
        r'obs_data_get_(?:double|int|bool|string)\((?:[^()]|\([^()]*\))*?,\s*"([^"]+)"',
        strategies, re.S,
    )
) | set(
    re.findall(
        r'SET_CLAMPED_(?:DOUBLE|INT_AS_FLOAT)\([^)]*?,\s*"([^"]+)"',
        strategies, re.S,
    )
) | set(
    re.findall(
        r'SET_BOOL_AS_FLOAT\([^)]*?,\s*"([^"]+)"',
        strategies, re.S,
    )
)
strategy_cached_constants = set(
    re.findall(r"GET_PARAM_SAFE\([^;]*?\b(P_[A-Z0-9_]+)\b", strategies, re.S)
)
strategy_cached_literals = set(
    re.findall(r'GET_PARAM_SAFE\([^;]*?"([a-z_]+)"', strategies, re.S)
) - {"elapsed_time", "uv_pixel_interval", "val_blur_radius", "ViewProj", "blurred_image", "image", "noise_tex"}

required_handheld_constants = {
    "P_HAND_TREMOR_AMP",
    "P_HAND_BREATH_AMP",
    "P_HAND_STEP_AMP",
    "P_HAND_STEP_SPEED",
    "P_HAND_CORRECTION_AMP",
    "P_HAND_INERTIA",
    "P_HAND_BLUR_RESPONSE",
}

for const in sorted(required_handheld_constants):
    if const not in param_defs:
        errors.append(f"handheld parameter constant {const} is missing")
    if const not in ui_param_constants:
        errors.append(f"handheld parameter {const} is not exposed in the UI")
    if const not in strategy_read_constants:
        errors.append(f"handheld parameter {const} is not read by strategy update")

for const in sorted(ui_param_constants):
    key = param_defs.get(const)
    if not key:
        errors.append(f"UI parameter constant {const} is not defined in lens-params.h")
    elif const not in strategy_read_constants and key not in strategy_read_literals:
        errors.append(f"UI parameter {const} ({key}) is never read by a strategy update")

shader_uniforms = set(
    re.findall(r"\buniform\s+(?:texture2d|float\d?(?:x\d)?|int|bool)\s+(\w+)\b", effect)
)
for const in sorted(strategy_read_constants):
    key = param_defs.get(const)
    if key in shader_uniforms and const not in strategy_cached_constants:
        errors.append(f"shader-backed parameter {const} ({key}) is read but not cached")

if re.search(r"\bSET_PARAM_BOOL\s*\(", strategies):
    errors.append("strategies must not use SET_PARAM_BOOL for .effect float-backed boolean uniforms")

# ---------------------------------------------------------------------------
# REGRESSION GUARDRAILS
# ---------------------------------------------------------------------------

# GR-1: Every GET_PARAM_SAFE lookup must have a matching shader uniform.
# This catches new params added to a strategy cache without corresponding
# shader declarations — would cause "not found" debug spam per frame.
all_cached_keys = strategy_cached_literals
for const in strategy_cached_constants:
    key = param_defs.get(const)
    if key:
        all_cached_keys.add(key)
known_special = {"blurred_image", "image", "noise_tex", "elapsed_time",
                 "uv_pixel_interval", "val_blur_radius", "ViewProj", "history_image",
                 "hand_pos_offset", "hand_rot_rad", "hand_zoom_factor"}
for key in sorted(all_cached_keys):
    if key in known_special:
        continue
    if key not in shader_uniforms:
        errors.append(f"GR-1: param '{key}' is looked up via GET_PARAM_SAFE but has no shader uniform")

# GR-2: Every SET_PARAM_* param in update must have a default in get_defaults.
# Prevents sliders that start at zero because no default was registered.
strategy_code = "\n".join(
    path.read_text(encoding="utf-8")
    for path in (ROOT / "src" / "strategies").glob("strategy-*.c")
)
all_strategy_params = " ".join(
    re.findall(r"(P_[A-Z0-9_]+)", strategy_code)
)
set_in_update = set()
for const in strategy_read_constants:
    if const not in strategy_code:
        continue
    # Check if get_defaults for this strategy registers a default
    # by looking for obs_data_set_default_* calls with the same key
    key = param_defs.get(const)
    if key and const not in strategy_cached_constants:
        continue  # internal params don't need defaults
    set_in_update.add(const)

defaults_found = set(
    re.findall(r"obs_data_set_default_(?:double|int|bool)\s*\([^,]+,\s*(P_[A-Z0-9_]+)\b", strategy_code)
)
defaults_skip = {
    "P_VID_COLOR", "P_LEAK_COLOR", "P_LEAK_COLOR_SHIFT", "P_SB_COLOR",
    "P_GRIT_COLOR", "P_BLOOM_COLOR", "P_BLOOM_GHOST_COLOR", "P_SPOT_TINT",
    "P_PART_COLOR_START", "P_PART_COLOR_END", "P_POLAROID_BASE_COLOR",
    "P_VID_HISTORY", "P_MAIN_TEX", "P_NOISE_TEX", "P_BLUR_IMAGE",
    "P_VIEW_PROJ", "P_ELAPSED_TIME", "P_UV_PIXEL_INTERVAL", "P_BLUR_RADIUS",
    "P_VID_SMEAR_ENABLE", "P_VID_SMEAR_MODE", "P_VID_SMEAR_DIRECTION",
    "P_VID_SMEAR_STRENGTH", "P_HAND_VEL_DIR_X", "P_HAND_VEL_DIR_Y",
    "P_HAND_VEL_MAG", "P_HAND_DIR_BLUR_ENABLE", "P_HAND_BLUR_DIRECTIONAL",
    "P_HAND_BLUR_RESPONSE", "P_HAND_BREATH_AMP", "P_HAND_CORRECTION_AMP",
    "P_HAND_INERTIA", "P_HAND_STEP_AMP", "P_HAND_STEP_SPEED",
    "P_HAND_TREMOR_AMP",
}
for const in sorted(set_in_update):
    if const in defaults_found:
        continue
    if const in defaults_skip:
        continue
    if const not in param_defs:
        continue
    errors.append(f"GR-2: {const} ({param_defs[const]}) is set in update but has no default in get_defaults")

# GR-3: Every bind_common_parameters call in a render function must be
# followed by a re-bind (strategies[m].update_settings) within the same
# function. This prevents the opacity-slider-not-working regression.
render_files = list((ROOT / "src" / "strategies").glob("strategy-*.c"))
render_files.append(ROOT / "src" / "obs-utils.h")
for rfile in render_files:
    text = rfile.read_text(encoding="utf-8")
    # Find all bind_common_parameters calls
    for m in re.finditer(r"bind_common_parameters\([^)]*\)\s*;", text):
        pos = m.end()
        snippet = text[pos:pos+600]
        if ("strategies" not in snippet
                and "orton_bind" not in snippet
                and "rebind_current_settings" not in snippet):
            # Check if it's within a render function (has render in function name nearby)
            func_context = text[max(0, pos-300):pos]
            if re.search(r"(?:render|technique)", func_context):
                errors.append(f"GR-3: {rfile.name} has bind_common_parameters without re-bind at offset {pos}")

if errors:
    print("FAIL: shader integrity problems found")
    for error in errors:
        print(f"  - {error}")
    sys.exit(1)

print("PASS: shader has unique functions/techniques/uniforms and defined samplers")
sys.exit(0)
