#!/usr/bin/env python3

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parent.parent


def extract_macro(text: str, name: str) -> str:
    start = text.find(f"#define {name}")
    if start == -1:
        return ""

    next_define = text.find("\n#define ", start + 1)
    return text[start:] if next_define == -1 else text[start:next_define]


def main() -> int:
    ui = (ROOT / "src/lens-ui.c").read_text()
    obs_utils = (ROOT / "src/obs-utils.c").read_text()
    params = (ROOT / "src/lens-params.h").read_text()

    errors = []

    section_match = re.search(
        r"static void section_header\([^)]*\) \{(?P<body>.*?)\n\}", ui, re.DOTALL
    )
    if not section_match:
        errors.append("section_header function not found")
    else:
        section_body = section_match.group("body")
        if "obs_properties_get" in section_body:
            errors.append(
                "section_header must not use obs_properties_get: scope mismatch -- "
                "obs_properties_get scoped to local container but has_prop checks "
                "entire hierarchy"
            )

    if "strstr(line, \"#include\")" in obs_utils:
        errors.append("shader include parser must not match embedded #include text")
    if not re.search(r"while \(\*p == ' ' \|\| \*p == '\\t'\)", obs_utils):
        errors.append("shader include parser must allow only leading whitespace before #include")

    diag = extract_macro(params, "SET_PARAM_DIAGNOSTIC")
    if not diag:
        errors.append("SET_PARAM_DIAGNOSTIC macro not found")
    else:
        category_pos = diag.find("info.type == GS_SHADER_PARAM_TEXTURE")
        optimized_pos = diag.find("actual_size == 0")
        if category_pos == -1 or optimized_pos == -1 or category_pos > optimized_pos:
            errors.append("SET_PARAM_DIAGNOSTIC must check texture/scalar category before optimized-out params")

    preset = extract_macro(params, "ADD_PRESET_ENTRY")
    if not preset:
        errors.append("ADD_PRESET_ENTRY macro not found")
    else:
        if "char _buf_pe[256]" in preset or "snprintf" in preset:
            errors.append("ADD_PRESET_ENTRY must not truncate localized labels into a fixed buffer")
        if "dstr_printf" not in preset or "dstr_free" not in preset:
            errors.append("ADD_PRESET_ENTRY should assemble labels with struct dstr")

    if errors:
        for error in errors:
            print(f"FAIL: {error}")
        return 1

    print("PASS: review contracts are enforced")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
