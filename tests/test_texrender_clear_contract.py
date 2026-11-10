#!/usr/bin/env python3
"""
Static regression test: verify all shared texrenders are cleared on mode switch.

Checks that lens-core.c contains:
- clear_all_shared_texrenders helper covering every shared texrender
- gs_texrender_reset called before gs_texrender_begin in the clear path
"""

import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
CORE_PATH = REPO / "src" / "lens-core.c"

core = CORE_PATH.read_text()

errors = []

# Every shared texrender field in struct lens_data that can hold stale data
required_targets = [
    "texrenders[0]",
    "texrenders[1]",
    "vidicon_history[0]",
    "vidicon_history[1]",
    "output_texrender",
    "blur_h_buffer",
    "blur_v_buffer",
    "bloom_anam_h",
    "bloom_anam_v",
    "input_texrender",
]

# Check for the shared clear helper
if "clear_all_shared_texrenders" not in core:
    errors.append("missing clear_all_shared_texrenders helper")

# Check every target field is referenced somewhere near the clear logic
# (we look for the array initializer body to avoid false matches from unrelated code)
clear_helper_start = core.find("static void clear_all_shared_texrenders(struct lens_data *filter) {")
if clear_helper_start >= 0:
    # Find the gs_texrender_t *targets[] = { ... } block
    bracket_start = core.find("gs_texrender_t *targets[]", clear_helper_start)
    if bracket_start >= 0:
        brace_open = core.find("{", bracket_start)
        brace_close = core.find("};", brace_open)
        block = core[brace_open:brace_close]
        for target in required_targets:
            if target not in block:
                errors.append(f"{target} not listed in clear_all_shared_texrenders targets array")
    else:
        errors.append("cannot find targets array in clear_all_shared_texrenders")
else:
    for target in required_targets:
        errors.append(f"{target} not covered by shared texrender clear path")

# Check reset-before-begin pattern: find clear_texrender_if_valid and verify
# gs_texrender_reset appears before gs_texrender_begin
helper_start = core.find("clear_texrender_if_valid")
if helper_start >= 0:
    reset_pos = core.find("gs_texrender_reset", helper_start)
    begin_pos = core.find("gs_texrender_begin", helper_start)
    if reset_pos < 0:
        errors.append("clear_texrender_if_valid does not call gs_texrender_reset")
    elif begin_pos < 0:
        errors.append("clear_texrender_if_valid does not call gs_texrender_begin")
    elif reset_pos > begin_pos:
        errors.append("gs_texrender_begin called before gs_texrender_reset in clear_texrender_if_valid")
else:
    errors.append("missing clear_texrender_if_valid helper")

# Recreated texrenders start without a backing texture until first begin/end.
# Feedback modes such as Vidicon must clear at the new dimensions immediately
# after resize so their first history read is transparent instead of falling
# back to the live source texture.
recreate_start = core.find("recreate_texrenders_if_needed")
if recreate_start >= 0:
    success_path = core[recreate_start:core.find("return TEXRENDER_RESIZED", recreate_start)]
    allocated_pos = success_path.find("filter->allocated_height = height")
    clear_pos = success_path.find("clear_all_shared_texrenders(filter)")
    if clear_pos < 0:
        errors.append("recreate_texrenders_if_needed does not clear texrenders after resize")
    elif allocated_pos < 0 or clear_pos < allocated_pos:
        errors.append("resize clear must run after allocated_width/height are updated")
else:
    errors.append("missing recreate_texrenders_if_needed helper")

if errors:
    print("FAIL: texrender clear contract violations:")
    for e in errors:
        print(f"  - {e}")
    sys.exit(1)
else:
    print("PASS: All shared texrenders covered by reset-before-begin clear path")
    sys.exit(0)
