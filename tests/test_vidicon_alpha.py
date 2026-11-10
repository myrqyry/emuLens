#!/usr/bin/env python3

import sys
from pathlib import Path


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    effect = (root / "data/emulens.effect").read_text()

    section_start = effect.find("float4 PSVidicon(")
    section_end = effect.find("float4 PSVidiconComposite(")
    if section_start == -1 or section_end == -1:
        print("FAIL: could not locate vidicon shader section")
        return 1

    vidicon = effect[section_start:section_end]
    if "vid_color.rgb" not in vidicon:
        print("FAIL: Vidicon color tint is missing")
        return 1

    print("PASS: Vidicon uses vid_color")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
