#!/usr/bin/env python3
"""Structural contract for the shared OBS helper layer."""

import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
HEADER = os.path.join(ROOT, "src", "obs-utils.h")
SOURCE = os.path.join(ROOT, "src", "obs-utils.c")

def read(path):
    with open(path, "r", encoding="utf-8") as f:
        return f.read()

def main():
    failures = 0
    header = read(HEADER)
    source = read(SOURCE)

    for helper in ("setting_visibility", "set_blending_parameters", "get_input_source"):
        pat = re.compile(rf"\b{helper}\s*\(")
        if not pat.search(header):
            print(f"FAIL: {helper} missing from obs-utils.h")
            failures += 1
        if not pat.search(source):
            print(f"FAIL: {helper} missing from obs-utils.c")
            failures += 1

    m = re.search(
        r"bool\s+get_input_source\([^)]*\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.S,
    )
    if not m:
        print("FAIL: could not isolate get_input_source")
        return 1

    body = m.group("body")
    required = (
        "gs_texrender_begin(filter->input_texrender",
        "obs_filter_get_target(filter->context)",
        "obs_filter_get_parent(filter->context)",
        "obs_source_get_output_flags(parent)",
        "obs_source_default_render(target)",
        "obs_source_video_render(target)",
        "gs_texrender_end(filter->input_texrender)",
    )
    for token in required:
        if token not in body:
            print(f"FAIL: get_input_source missing {token}")
            failures += 1

    for forbidden in (
        "obs_source_process_filter_begin",
        "obs_source_process_filter_end",
        "OBS_NO_DIRECT_RENDERING",
    ):
        if forbidden in body:
            print(f"FAIL: nested filter-processing capture reintroduced: {forbidden}")
            failures += 1

    branch = re.search(
        r"if\s*\(target\s*==\s*parent\s*&&\s*!parent_custom_draw\s*&&\s*!parent_async\)"
        r"\s*obs_source_default_render\(target\);\s*else\s*obs_source_video_render\(target\);",
        body,
        re.S,
    )
    if not branch:
        print("FAIL: ordinary-source/default-render branch does not match OBS contract")
        failures += 1

    if failures:
        print(f"FAILED: {failures} obs-utils contract violation(s)")
        return 1

    print("PASS: source capture uses OBS-native default/video render branching")
    return 0

if __name__ == "__main__":
    sys.exit(main())
