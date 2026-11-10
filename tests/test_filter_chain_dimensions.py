#!/usr/bin/env python3
"""Regression guard for stacked-filter dimension handling."""

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
CORE = (ROOT / "src" / "lens-core.c").read_text(encoding="utf-8")
errors = []

info = re.search(
    r"struct obs_source_info emulens_filter_info\s*=\s*\{(?P<body>.*?)\n\};",
    CORE,
    re.S,
)
if not info:
    errors.append("could not find emulens_filter_info")
else:
    body = info.group("body")
    if ".get_width" in body or ".get_height" in body:
        errors.append(
            "dimension-preserving filters must let libobs inherit target size; "
            "custom width/height callbacks can query filter_target outside its "
            "documented valid callbacks"
        )

if re.search(r"static\s+uint32_t\s+lens_get_(width|height)\s*\(", CORE):
    errors.append("obsolete lens_get_width/lens_get_height callback remains")

helper = re.search(
    r"static bool lens_get_target_base_size\([^)]*\)\s*\{(?P<body>.*?)\n\}",
    CORE,
    re.S,
)
if not helper:
    errors.append("could not find render-scoped target-base dimension helper")
else:
    body = helper.group("body")
    if "obs_filter_get_target" not in body:
        errors.append("dimension helper does not follow the immediate filter target")
    if "obs_source_get_base_width" not in body or "obs_source_get_base_height" not in body:
        errors.append("dimension helper must use target base dimensions")
    if re.search(r"obs_source_get_(width|height)\s*\(", body):
        errors.append("dimension helper must not use recursive source width/height")

render = re.search(
    r"static bool lens_get_render_size\([^)]*\)\s*\{(?P<body>.*?)\n\}",
    CORE,
    re.S,
)
if not render:
    errors.append("missing lens_get_render_size")
elif "lens_get_target_base_size" not in render.group("body"):
    errors.append("video_render dimension lookup does not use target base size")

if "lens_get_render_size(filter, &base_w, &base_h)" not in CORE:
    errors.append("video_render does not use the render-scoped dimension helper")

tick = re.search(
    r"static void lens_tick\([^)]*\)\s*\{(?P<body>.*?)\n\}",
    CORE,
    re.S,
)
if tick and (
    "obs_filter_get_target" in tick.group("body")
    or "obs_filter_get_parent" in tick.group("body")
):
    errors.append("video_tick must not query filter parent/target dimensions")

if errors:
    for error in errors:
        print(f"FAIL: {error}")
    sys.exit(1)

print("PASS: stacked filters inherit dimensions; render probes use non-recursive base sizes")
