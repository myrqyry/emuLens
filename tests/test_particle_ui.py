#!/usr/bin/env python3

import re

from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent


def main() -> int:
    source = (ROOT / "src/lens-ui.c").read_text()
    normalized = re.sub(r"\s+", "", source)

    required = "obs_properties_add_float_slider(props,P_PART_SPEED"
    if required not in normalized:
        print("FAIL: missing particle speed slider in UI")
        return 1

    print("PASS: particle speed slider is exposed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
