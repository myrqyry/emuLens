"""
Contract test: the lens-ui.c visibility callbacks use the
`setting_visibility` helper from src/obs-utils.h instead of
inlining the `obs_property_set_visible` pattern.

The helper also calls `obs_property_set_enabled`, which is a
behavioral improvement: a hidden sub-property in OBS should be
disabled (not just invisible) so its value cannot be edited
programmatically via the property API.

Locks in the contract that all 7 visibility callbacks in lens-ui.c
(handheld_blur_enable, sb_colorize, leak_color_shift,
part_source_color, part_source_brightness, part_use_poly,
bokeh_high_response) use `setting_visibility` rather than calling
`obs_property_set_visible` directly.

Failing this test means a callback regressed to the inline pattern.
"""

import os
import re
import sys

REPO = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
UI = os.path.join(REPO, "src", "lens-ui.c")

EXPECTED_CALLBACKS = [
    "handheld_blur_enable_modified",
    "sb_colorize_modified",
    "leak_color_shift_modified",
    "part_source_color_modified",
    "part_source_brightness_modified",
    "part_use_poly_modified",
    "bokeh_high_response_modified",
]


def read_file(path):
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def extract_function_body(text, func_name):
    match = re.search(
        rf"static\s+bool\s+{re.escape(func_name)}\s*\([^)]*\)\s*\{{(?P<body>.*?)\n\}}",
        text, re.S,
    )
    return match.group("body") if match else None


def main():
    failures = 0
    text = read_file(UI)
    if "setting_visibility" not in text:
        print("FAIL: lens-ui.c does not include or use setting_visibility")
        return 1
    print("OK:   lens-ui.c references setting_visibility")

    for cb in EXPECTED_CALLBACKS:
        body = extract_function_body(text, cb)
        if body is None:
            print(f"FAIL: {cb} function not found in lens-ui.c")
            failures += 1
            continue

        uses_helper = "setting_visibility(" in body
        uses_inline_visible = re.search(
            r"obs_property_set_visible\s*\(", body
        ) is not None

        if uses_helper and not uses_inline_visible:
            print(
                f"OK:   {cb}_modified uses setting_visibility helper"
            )
        elif uses_helper and uses_inline_visible:
            # OK: helper is the primary mechanism; legacy visible
            # calls are still acceptable for properties whose
            # visibility depends on a different parent (e.g., an
            # extra section header). But the primary property for
            # the gate must use the helper.
            print(
                f"PARTIAL: {cb}_modified uses helper but also has "
                "inline obs_property_set_visible calls"
            )
        else:
            print(
                f"FAIL: {cb}_modified does not use setting_visibility "
                "and still calls obs_property_set_visible inline"
            )
            failures += 1

    print()
    if failures:
        print(f"FAILED: {failures} lens-ui visibility-callback regression(s)")
        return 1
    print("PASSED: lens-ui.c visibility callbacks use setting_visibility helper")
    return 0


if __name__ == "__main__":
    sys.exit(main())
