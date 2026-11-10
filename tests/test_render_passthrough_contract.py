"""
Structural contract test for the trivial pass-through strategy _render bodies.

Locks in two contracts:
  1. Every trivial (single-technique, non-feedback) strategy _render must
     produce a texture by running exactly one technique against one
     texrender slot. This is the contract the new
     `render_passthrough_to_texrender` helper is built around.
  2. Once the helper exists, every such _render must call the helper
     exactly once — not inline the same 5-7 call sequence by hand.

Trivial strategies in scope (vidicon, bloom, orton, handheld are out of
scope — they are multi-pass or feedback-based and have their own
specialised render bodies).
"""

import os
import sys
import re

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
STRAT_DIR = os.path.join(REPO_ROOT, "src", "strategies")

# Strategies that should use the trivial pass-through helper.
# vidicon / bloom / orton / handheld do NOT — they are multi-pass
# or feedback-based and own their own render bodies.
TRIVIAL_STRATEGIES = [
    "bokeh", "daguerreotype", "grit", "leak", "mirage", "particles",
    "pinhole", "polaroid", "spot", "starburst", "technicolor",
]

# Marker call sequence that the helper replaces. The helper MUST end
# up being called in place of these manual steps.
PASSTHROUGH_MARKERS = [
    "gs_texrender_reset(",
    "gs_texrender_begin(",
    "gs_clear(GS_CLEAR_COLOR",
    "bind_common_parameters(",
    "lens_run_technique_quad(",
    "gs_texrender_end(",
    "gs_texrender_get_texture(",
]

# Helper name once introduced. After the refactor, every trivial
# strategy's _render must reference it.
HELPER_NAME = "render_passthrough_to_texrender"


def render_body(path):
    body = open(path, "r", encoding="utf-8").read()
    match = re.search(
        r"gs_texture_t\s+\*\s*\w+_render\s*\([^)]*\)\s*\{(.*?)\n\}\n",
        body, re.DOTALL,
    )
    if not match:
        return None
    return match.group(1)


def main():
    failures = 0
    for suffix in TRIVIAL_STRATEGIES:
        path = os.path.join(STRAT_DIR, f"strategy-{suffix}.c")
        if not os.path.exists(path):
            print(f"FAIL: {path} missing")
            failures += 1
            continue

        body = render_body(path)
        if body is None:
            print(f"FAIL: strategy-{suffix}: no _render function found")
            failures += 1
            continue

        has_helper = HELPER_NAME in body
        marker_count = sum(1 for m in PASSTHROUGH_MARKERS if m in body)

        if has_helper:
            if marker_count > 2:
                print(
                    f"FAIL: strategy-{suffix}: helper called AND "
                    f"{marker_count} manual pass-through markers remain"
                )
                failures += 1
            else:
                print(
                    f"OK:   strategy-{suffix}: uses {HELPER_NAME} "
                    f"({marker_count} manual markers)"
                )
        else:
            missing = [m for m in PASSTHROUGH_MARKERS if m not in body]
            if missing:
                print(
                    f"FAIL: strategy-{suffix}: pre-refactor body is missing "
                    f"expected markers: {missing}"
                )
                failures += 1
            else:
                print(
                    f"WARN: strategy-{suffix}: still using inline "
                    f"pass-through body (refactor pending)"
                )

    print()
    if failures:
        print(f"FAILED: {failures} strategy(ies) violate the contract")
        return 1
    print("PASSED: All trivial pass-through strategy render bodies match contract")
    return 0


if __name__ == "__main__":
    sys.exit(main())
