#!/usr/bin/env python3

from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent


def main() -> int:
    effect = (ROOT / "data/emulens.effect").read_text()
    if "mir_rot_speed * elapsed_time * 0.1" not in effect:
        print("FAIL: mirage rotation speed is not scaled down")
        return 1

    print("PASS: mirage rotation speed is scaled down")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
