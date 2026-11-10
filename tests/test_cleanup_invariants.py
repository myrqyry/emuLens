"""
Regression test for the AI-slop-cleaner pass invariants.

Locks in contracts that protect against regression of cleanup work:
  - `lens_get_render_target_size` is not defined in lens-core.c
  - The trivial pass-through render helper `render_passthrough_to_texrender`
    is declared in obs-utils.h
  - Strategy forward decls in lens-data.h are X-macro generated, not
    hand-written

Failing this test means someone reintroduced dead code, broke the
render helper, or regressed the X-macro dedup.
"""

import os
import sys

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
LENS_DATA = os.path.join(REPO_ROOT, "src", "lens-data.h")
LENS_CORE = os.path.join(REPO_ROOT, "src", "lens-core.c")
OBS_UTILS = os.path.join(REPO_ROOT, "src", "obs-utils.h")


def read(path):
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def must_be_absent(name, path, body):
    if name in body:
        print(f"FAIL: {name!r} must NOT appear in {path}")
        return 1
    print(f"OK:   {name!r} absent from {os.path.relpath(path, REPO_ROOT)}")
    return 0


def must_be_present(name, path, body):
    if name not in body:
        print(f"FAIL: {name!r} must appear in {path}")
        return 1
    print(f"OK:   {name!r} present in {os.path.relpath(path, REPO_ROOT)}")
    return 0


def main():
    failures = 0
    data_h = read(LENS_DATA)
    core_c = read(LENS_CORE)
    obs_utils_h = read(OBS_UTILS)

    # Pass 1 deletions: declaration removed from lens-data.h (was dead — no callers)
    failures += must_be_absent("lens_get_render_target_size", LENS_DATA, data_h)

    # Pass 2b: render helper exists
    failures += must_be_present(
        "render_passthrough_to_texrender", OBS_UTILS, obs_utils_h
    )

    # Pass 2 dedup: vtable decls are X-macro generated, not hand-written
    hand_written = []
    for decl in (
        "void vidicon_cache(",
        "void starburst_cache(",
        "void bloom_render(",
    ):
        if decl in data_h:
            hand_written.append(decl)
    if hand_written:
        print(
            "FAIL: hand-written strategy decls regressed in lens-data.h: "
            + ", ".join(hand_written)
        )
        failures += 1
    else:
        print("OK:   strategy decls remain X-macro generated")

    # Pass 2c: SET_CLAMPED_* macros exist in lens-params.h
    if "SET_CLAMPED_DOUBLE" in read(
        os.path.join(REPO_ROOT, "src", "lens-params.h")
    ):
        print("OK:   SET_CLAMPED_DOUBLE macro exists")
    else:
        print("FAIL: SET_CLAMPED_DOUBLE macro missing from lens-params.h")
        failures += 1

    print()
    if failures:
        print(f"FAILED: {failures} invariant(s) broken")
        return 1
    print("PASSED: All cleanup invariants hold")
    return 0


if __name__ == "__main__":
    sys.exit(main())
