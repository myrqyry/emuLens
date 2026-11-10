#!/usr/bin/env python3

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parent.parent


def main() -> int:
    source = (ROOT / "src/strategies/strategy-grit.c").read_text()

    expected = {
        'P_GRIT_DUST', 'P_GRIT_SCRATCHES', 'P_GRIT_NOISE', 'P_GRIT_BRIGHTNESS',
        'P_GRIT_CONTRAST', 'P_GRIT_SATURATION', 'P_GRIT_OPACITY', 'P_GRIT_BLEND_MODE',
        'P_GRIT_COLOR',
    }

    found = set(re.findall(r'obs_data_set_default_(?:double|int)\(settings, (P_GRIT_\w+)', source))
    missing = expected - found
    if missing:
        print(f"FAIL: missing grit defaults: {sorted(missing)}")
        return 1

    print("PASS: grit defaults are sensible")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
