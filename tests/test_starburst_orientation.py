#!/usr/bin/env python3

import re
import sys
from pathlib import Path


def read_text(path: Path) -> str:
    if not path.exists():
        raise FileNotFoundError(path)
    return path.read_text()


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    ui = read_text(root / "src/lens-ui.c")
    effect = read_text(root / "data/emulens.effect")

    if "P_SB_ROTATION" not in ui:
        print("FAIL: missing starburst rotation property")
        return 1

    if not re.search(r"P_SB_ROTATION,\s*obs_module_text\(\"Rotation\"\)", ui):
        print("FAIL: starburst rotation property label missing")
        return 1

    if not re.search(r"P_SB_ROTATION.*0\.0,\s*360\.0", ui, re.S):
        print("FAIL: starburst rotation slider range is not 0..360")
        return 1

    if "uniform float sb_rotation" not in effect:
        print("FAIL: missing starburst rotation uniform")
        return 1

    if "sb_rotation * 0.01745329252" not in effect or "sb_rot_speed * elapsed_time" not in effect:
        print("FAIL: starburst rotation is not combined with animation speed")
        return 1

    print("PASS: starburst rotation control verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
