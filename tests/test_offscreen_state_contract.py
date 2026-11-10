#!/usr/bin/env python3
from pathlib import Path
import sys

s=(Path(__file__).resolve().parents[1]/"src/obs-utils.h").read_text()
start=s.index("render_passthrough_to_texrender")
body=s[start:s.index("return gs_texrender_get_texture", start)]
errors=[]
for token in (
    "set_blending_parameters();",
    "gs_blend_state_pop();",
    "gs_texrender_begin(output_buf",
    "gs_texrender_end(output_buf)",
):
    if token not in body:
        errors.append(f"missing {token}")
if body.find("set_blending_parameters();") > body.find("lens_run_technique_quad"):
    errors.append("blend state normalized after draw instead of before")
if errors:
    for e in errors: print("FAIL:",e)
    sys.exit(1)
print("PASS: single-pass offscreen renders isolate blend state")
