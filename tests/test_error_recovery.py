#!/usr/bin/env python3
"""
Error recovery path tests for emuLensFX.

Verifies blur fallback, param validation, and failure guards.
"""

import os
import re
import sys

REPO = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
CORE_PATH = os.path.join(REPO, "src", "lens-core.c")
UTILS_PATH = os.path.join(REPO, "src", "obs-utils.c")

errors = []


def read_file(path):
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def check():
    text = read_file(CORE_PATH) + "\n"
    try:
        text += read_file(UTILS_PATH)
    except FileNotFoundError:
        pass

    # 1. apply_gaussian_blur short-circuits when radius is below minimum
    if re.search(r'radius\s*<\s*MIN_BLUR_RADIUS', text):
        print("  PASS: apply_gaussian_blur short-circuits on radius below minimum")
    else:
        errors.append("apply_gaussian_blur missing radius below minimum guard")

    # 2. apply_gaussian_blur uses the current input texture as the fallback
    if re.search(r'return tex;', text):
        print("  PASS: apply_gaussian_blur falls back to the input texture")
    else:
        errors.append("apply_gaussian_blur missing input texture fallback")

    # 3. Dead cached_radius field removed (no false caching impression)
    if not re.search(r'cached_radius\s*=', text):
        print("  PASS: Dead cached_radius field removed from blur path")
    else:
        errors.append("cached_radius still present (dead code not cleaned up)")

    # 4. lens_render checks critical_params_valid
    if re.search(r'critical_params_valid', text):
        print("  PASS: lens_render checks critical_params_valid")
    else:
        errors.append("lens_render missing critical_params_valid check")

    # 5. lens_render calls obs_source_skip_video_filter on param failure
    if re.search(r'!filter->param_validation.critical_params_valid', text):
        print("  PASS: Param validation failure skips frame")
    else:
        errors.append("Missing critical_params_valid skip guard")

    # 6. get_input_source (formerly capture_input_texture) bails out if
    # gs_texrender_begin fails.
    if re.search(r'gs_texrender_begin\(filter->input_texrender', text):
        print("  PASS: get_input_source guards texrender_begin")
    else:
        errors.append("get_input_source missing texrender_begin guard")

    # 7. lens_create failure path has proper cleanup
    if re.search(r'safe_texrender_destroy\(&filter->texrenders\[0\]\)', text):
        print("  PASS: lens_create failure path uses safe_texrender_destroy")
    else:
        errors.append("lens_create failure path missing safe_texrender_destroy")

    # 8. lens_destroy has null guard
    if re.search(r'if\s*\(!filter\)\s*\n\s*return', text):
        print("  PASS: lens_destroy has null data guard")
    else:
        errors.append("lens_destroy missing null data guard")

    # 9. apply_gaussian_blur handles null buffers
    if re.search(r'!filter->blur_h_buffer\s*\|\|\s*!filter->blur_v_buffer', text):
        print("  PASS: apply_gaussian_blur guards null buffers")
    else:
        errors.append("apply_gaussian_blur missing null buffer guard")

    # 10. apply_gaussian_blur returns tex on tiny radius
    if re.search(r'radius\s*<\s*MIN_BLUR_RADIUS', text):
        print("  PASS: apply_gaussian_blur short-circuits on tiny radius")
    else:
        errors.append("apply_gaussian_blur missing tiny-radius short-circuit")


def main():
    print("=" * 60)
    print("emuLensFX Error Recovery Path Test")
    print("=" * 60)
    check()
    print("-" * 60)
    if errors:
        for e in errors:
            print(f"  FAIL: {e}")
        sys.exit(1)
    print("ALL CHECKS PASSED")


if __name__ == "__main__":
    main()
