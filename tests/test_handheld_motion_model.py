#!/usr/bin/env python3

import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
PARAMS = (ROOT / "src/lens-params.h").read_text(encoding="utf-8")
DATA = (ROOT / "src/lens-data.h").read_text(encoding="utf-8")
UI = (ROOT / "src/lens-ui.c").read_text(encoding="utf-8")
STRATEGY = (ROOT / "src/strategies/strategy-handheld.c").read_text(
    encoding="utf-8"
)
LOCALE = (ROOT / "data/locale/en-US.ini").read_text(encoding="utf-8")
EFFECT = (ROOT / "data/emulens.effect").read_text(encoding="utf-8")


NEW_PARAMS = {
    "P_HAND_TREMOR_AMP": "hand_tremor_amp",
    "P_HAND_BREATH_AMP": "hand_breath_amp",
    "P_HAND_STEP_AMP": "hand_step_amp",
    "P_HAND_STEP_SPEED": "hand_step_speed",
    "P_HAND_CORRECTION_AMP": "hand_correction_amp",
    "P_HAND_INERTIA": "hand_inertia",
    "P_HAND_BLUR_RESPONSE": "hand_blur_response",
}

NEW_RUNTIME_FIELDS = [
    "tremor_amp",
    "breath_amp",
    "step_amp",
    "step_speed",
    "correction_amp",
    "inertia",
    "blur_response",
    "smoothed_offset",
    "prev_offset",
    "smoothed_rot_rad",
    "prev_rot_rad",
    "smoothed_zoom_factor",
    "prev_zoom_factor",
    "prev_elapsed_sec",
    "blur_velocity",
    "motion_initialized",
]

NEW_LOCALE_KEYS = [
    "TremorAmount",
    "TremorAmountTooltip",
    "BreathingAmount",
    "BreathingAmountTooltip",
    "StepBobAmount",
    "StepBobAmountTooltip",
    "StepBobSpeed",
    "StepBobSpeedTooltip",
    "CorrectionAmount",
    "CorrectionAmountTooltip",
    "InertiaAmount",
    "InertiaAmountTooltip",
    "BlurResponse",
    "BlurResponseTooltip",
]

PRESETS = [
    "default_handheld.json",
    "handheld_breathing.json",
    "handheld_drift.json",
    "handheld_run.json",
]


def require(condition, message):
    if not condition:
        print(f"FAIL: {message}")
        sys.exit(1)


def main():
    for const, key in NEW_PARAMS.items():
        require(f'#define {const} "{key}"' in PARAMS, f"missing {const}")
        require(
            f"obs_data_get_double(settings, {const})" in STRATEGY,
            f"strategy does not read {const}",
        )
        require(
            f"obs_data_set_default_double(settings, {const}" in STRATEGY,
            f"strategy does not set default for {const}",
        )
        require(
            re.search(rf"obs_properties_add_float_slider\([^;]+{const}", UI, re.S),
            f"UI does not expose {const}",
        )

    for field in NEW_RUNTIME_FIELDS:
        require(field in DATA, f"missing runtime field {field}")
        require(field in STRATEGY, f"strategy does not use {field}")

    for key in NEW_LOCALE_KEYS:
        require(f"{key}=\"" in LOCALE, f"missing locale key {key}")
        require(f'obs_module_text("{key}")' in UI, f"UI does not use {key}")

    require("handheld_signed_noise" in STRATEGY, "missing signed noise helper")
    require("handheld_lerp" in STRATEGY, "missing interpolation helper")
    require("motion_velocity" in STRATEGY, "missing velocity-derived blur signal")
    require("expf(-responsiveness * dt)" in STRATEGY, "missing inertia smoothing")
    require("blur_velocity" in STRATEGY, "missing smoothed blur velocity")
    require(
        "sinf(elapsed_sec * filter->runtime_state.handheld_blur_speed"
        not in STRATEGY,
        "dynamic blur still uses the old sine-only pulse",
    )

    require("technique DrawHandheld" in EFFECT, "DrawHandheld technique missing")
    require("VSHandheld" not in EFFECT, "custom handheld vertex shader was reintroduced")
    require("uv.y = 1.0 - uv.y" not in EFFECT, "handheld shader flips vertically")
    require("gl_" not in EFFECT, "shader uses reserved gl_* identifiers")
    require("register(" not in EFFECT, "shader uses explicit hardware registers")

    for preset_name in PRESETS:
        preset_path = ROOT / "data/presets" / preset_name
        preset = json.loads(preset_path.read_text(encoding="utf-8"))
        for key in NEW_PARAMS.values():
            require(key in preset, f"{preset_name} missing {key}")

    print("PASS: handheld motion model exposes authentic controls and velocity blur")


if __name__ == "__main__":
    main()
