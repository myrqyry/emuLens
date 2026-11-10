#!/usr/bin/env python3
"""Static regression test for Vidicon feedback-buffer ownership."""

import re
import sys
from pathlib import Path


REPO = Path(__file__).resolve().parent.parent
CORE = REPO / "src" / "lens-core.c"
STRATEGIES_H = REPO / "src" / "strategies" / "strategies.h"
VIDICON = REPO / "src" / "strategies" / "strategy-vidicon.c"


def function_body(source: str, name: str) -> str:
    match = re.search(rf"\bvoid\s+{name}\s*\([^)]*\)\s*\{{", source)
    if not match:
        raise AssertionError(f"missing function {name}")
    start = match.end()
    depth = 1
    index = start
    while index < len(source) and depth:
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
        index += 1
    if depth:
        raise AssertionError(f"unterminated function {name}")
    return source[start : index - 1]


def main() -> int:
    errors = []
    core = CORE.read_text()
    strategies_h = STRATEGIES_H.read_text()
    vidicon = VIDICON.read_text()

    if "vidicon_history" not in strategies_h:
        errors.append(
            "strategies.h ownership rules must explicitly document that "
            "vidicon_history[] is owned by lens-core.c"
        )

    for index in (0, 1):
        create = f"filter->vidicon_history[{index}] = gs_texrender_create"
        if create not in core:
            errors.append(f"lens-core.c must create vidicon_history[{index}]")

        destroy = f"safe_texrender_destroy(&filter->vidicon_history[{index}])"
        if destroy not in core:
            errors.append(f"lens-core.c must destroy vidicon_history[{index}]")

    destroy_body = function_body(vidicon, "vidicon_destroy")
    if "gs_texrender_destroy" in destroy_body or "safe_texrender_destroy" in destroy_body:
        errors.append(
            "vidicon_destroy must not destroy vidicon_history[]; those shared "
            "buffers are centrally owned by lens-core.c"
        )

    if errors:
        print("FAIL: Vidicon lifecycle contract violations:")
        for error in errors:
            print(f"  - {error}")
        return 1

    print("PASS: Vidicon feedback buffers are centrally owned by lens-core.c")
    return 0


if __name__ == "__main__":
    sys.exit(main())
