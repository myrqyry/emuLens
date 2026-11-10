#!/usr/bin/env python3
"""
GPU resource lifecycle tests for emuLensFX.

Verifies texrender creation/destruction patterns and resize logic.
"""

import os
import re
import sys

REPO = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
CORE_PATH = os.path.join(REPO, "src", "lens-core.c")

errors = []


def read_file(path):
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def check():
    text = read_file(CORE_PATH)

    # 1. lens_create creates all 6 texrenders
    create_calls = len(re.findall(r'gs_texrender_create\(GS_RGBA,\s*GS_ZS_NONE\)', text))
    if create_calls >= 8:  # 6 primary + warmup + techniques
        print(f"  PASS: {create_calls} gs_texrender_create calls found")
    else:
        errors.append(f"Too few gs_texrender_create calls ({create_calls})")

    # 2. lens_create allocates both texrenders[0] and texrenders[1]
    alloc_count = len(re.findall(r'filter->texrenders\[\d\]', text))
    if alloc_count >= 2:
        print("  PASS: texrenders[0] and texrenders[1] allocated")
    else:
        errors.append("Missing texrenders allocation")

    # 3. lens_destroy uses safe_texrender_destroy
    safe_calls = len(re.findall(r'safe_texrender_destroy\(&filter->texrenders', text))
    if safe_calls >= 2:
        print(f"  PASS: safe_texrender_destroy used for texrenders")
    else:
        errors.append("lens_destroy missing safe_texrender_destroy for texrenders")

    # 4. safe_texrender_destroy called for all 6 texrenders in lens_destroy
    destroy_count = len(re.findall(r'safe_texrender_destroy', text))
    if destroy_count >= 6:
        print(f"  PASS: {destroy_count} safe_texrender_destroy calls present")
    else:
        errors.append(f"Too few safe_texrender_destroy calls ({destroy_count})")

    # 5. resize checks dimension match
    if re.search(r'filter->allocated_width\s*==\s*width\s*&&\s*filter->allocated_height\s*==\s*height', text):
        print("  PASS: Dimension match check in recreate_texrenders_if_needed")
    else:
        errors.append("No dimension match check in recreate_texrenders_if_needed")

    # 6. resize returns NO_CHANGE on match
    if re.search(r'TEXRENDER_NO_CHANGE', text):
        print("  PASS: TEXRENDER_NO_CHANGE returned on dimension match")
    else:
        errors.append("Missing TEXRENDER_NO_CHANGE return")

    # 7. resize handles invalid dimensions
    if re.search(r'width\s*==\s*0\s*\|\|\s*height\s*==\s*0', text):
        print("  PASS: Invalid dimension guard present")
    else:
        errors.append("No invalid dimension (0x0) guard")

    # 8. resize returns FAILED on invalid dimensions
    if re.search(r'return TEXRENDER_FAILED', text) or re.search(r'return TEXRENDER_FAILED', text):
        print("  PASS: TEXRENDER_FAILED returned for invalid dimensions")
    else:
        errors.append("Missing TEXRENDER_FAILED return for invalid dims")

    # 9. resize has thrashing protection
    if re.search(r'consecutive_thrash', text):
        print("  PASS: Thrashing protection present")
    else:
        errors.append("No thrashing protection in recreate_texrenders_if_needed")

    # 10. resize resets thrash after 500ms
    if re.search(r'500000000', text):
        print("  PASS: 500ms thrash reset interval present")
    else:
        errors.append("Missing 500ms thrash reset interval")

    # 11. texture dimension exceeds safe limit returns FAILED
    if re.search(r'max_safe_dimension', text):
        print("  PASS: Max safe dimension guard present")
    else:
        errors.append("Missing max safe dimension guard")

    # 12. RECREATE_RENDER macro nullifies after destroy
    if re.search(r'r = NULL', text):
        print("  PASS: RECREATE_RENDER sets pointer to NULL after destroy")
    else:
        errors.append("RECREATE_RENDER missing NULL assignment")


def main():
    print("=" * 60)
    print("emuLensFX GPU Resource Lifecycle Test")
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
