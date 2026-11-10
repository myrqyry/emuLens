"""
Contract test for the lens_update mode-switch X-macro dispatch.

Locks in the contract that `lens_update`'s per-mode activation
(`cache_params`) is driven by the `EMULENS_MODES` X-macro rather than a
hand-rolled `if (strategies[new_mode].cache_params)` runtime check.
`update_settings` is intentionally called from `lens_render` every frame,
where shader parameter bindings are valid for the render path.

The X-macro has a known shape:
  #define X(id, suffix, ...) ... strategies[MODE_##id]...
  EMULENS_MODES
  #undef X

Failing this test means the dispatch regressed to per-strategy
if-checks, which adds a runtime branch for every existing and future
mode instead of letting the compiler expand the calls inline.
"""

import os
import re
import sys

REPO = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
CORE_PATH = os.path.join(REPO, "src", "lens-core.c")
DATA_PATH = os.path.join(REPO, "src", "lens-data.h")


def read_file(path):
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def find_lens_update(text):
    """Extract the body of `static void lens_update(...)` for inspection."""
    match = re.search(r"static\s+void\s+lens_update\s*\([^)]*\)\s*\{",
                      text)
    if not match:
        return ""
    depth = 1
    index = match.end()
    while index < len(text) and depth:
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
        index += 1
    return text[match.end() : index - 1] if depth == 0 else ""


def main():
    failures = 0
    core = read_file(CORE_PATH)
    data = read_file(DATA_PATH)

    # Sanity: EMULENS_MODES must still exist in lens-data.h.
    if "EMULENS_MODES" not in data:
        print("FAIL: EMULENS_MODES X-macro missing from lens-data.h")
        return 1
    print("OK:   EMULENS_MODES X-macro present in lens-data.h")

    body = find_lens_update(core)
    if not body:
        print("FAIL: lens_update function not found in lens-core.c")
        return 1

    # The X-macro dispatch must live inside lens_update and reference
    # `cache_params` in the macro body. Settings updates are intentionally
    # handled by lens_render rather than this mode-switch activation path.
    macro_blocks = re.findall(
        r"#define\s+X\([^)]*\)(?P<body>.*?)#undef\s+X", body, re.S
    )
    has_macro_define = any("cache_params" in block for block in macro_blocks)
    has_macro_use = "EMULENS_MODES" in body and re.search(
        r"#undef\s+X", body
    ) is not None
    has_old_runtime_check = re.search(
        r"if\s*\(\s*strategies\[(?:new_mode|old_mode)\]\s*\.\s*cache_params\s*\)",
        body,
    ) is not None

    if has_macro_define and has_macro_use and not has_old_runtime_check:
        print("OK:   lens_update uses EMULENS_MODES X-macro dispatch for "
              "cache_params")
    else:
        details = []
        if not has_macro_define:
            details.append(
                "no #define X(...) { cache_params }"
            )
        if not has_macro_use:
            details.append("no EMULENS_MODES use + matching #undef X")
        if has_old_runtime_check:
            details.append("still has if (strategies[...].cache_params)")
        print("FAIL: lens_update does not use X-macro dispatch: "
              + "; ".join(details))
        failures += 1

    print()
    if failures:
        print(f"FAILED: {failures} lens_update dispatch contract violation(s)")
        return 1
    print("PASSED: lens_update mode-switch uses EMULENS_MODES X-macro")
    return 0


if __name__ == "__main__":
    sys.exit(main())
