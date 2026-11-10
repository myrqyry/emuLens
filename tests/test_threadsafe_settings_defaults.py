#!/usr/bin/env python3

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parent.parent


def main() -> int:
    core = (ROOT / "src/lens-core.c").read_text()
    polaroid = (ROOT / "src/strategies/strategy-polaroid.c").read_text()

    errors = []

    lens_update = re.search(
        r"static void lens_update\(void \*data, obs_data_t \*settings\) \{(?P<body>.*?)\n\}",
        core,
        re.DOTALL,
    )
    if not lens_update:
        errors.append("lens_update function not found")
    else:
        body = lens_update.group("body")
        if "obs_data_create()" not in body or "obs_data_apply" not in body:
            errors.append("lens_update must cache an isolated obs_data_t copy")
        defaults_pos = body.find("emulens_get_defaults(settings_copy)")
        apply_pos = body.find("obs_data_apply(settings_copy, settings)")
        if defaults_pos == -1 or apply_pos == -1 or defaults_pos > apply_pos:
            errors.append("lens_update settings snapshot must be seeded with defaults before apply")
        if re.search(r"obs_data_addref\(settings\).*filter->cached_settings\s*=\s*settings", body, re.DOTALL):
            errors.append("lens_update must not cache the mutable update settings handle")

    color_default = re.search(
        r"obs_data_set_default_int\(settings,\s*P_POLAROID_BASE_COLOR,\s*(0x[0-9A-Fa-f]+)\)",
        polaroid,
    )
    if not color_default:
        errors.append("Polaroid base color default not found")
    elif int(color_default.group(1), 16) >> 24 != 0xFF:
        errors.append("Polaroid base color default must include opaque alpha")

    if errors:
        for error in errors:
            print(f"FAIL: {error}")
        return 1

    print("PASS: settings caching and Polaroid color defaults are safe")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
