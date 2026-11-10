#!/usr/bin/env python3

from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent


def main() -> int:
    source = (ROOT / "src/strategies/strategy-spot.c").read_text()

    required = [
        'obs_data_set_default_double(settings, P_SPOT_RADIUS, 0.15);',
        'obs_data_set_default_double(settings, P_SPOT_SOFTNESS, 0.14);',
        'obs_data_set_default_double(settings, P_SPOT_ZOOM_STRENGTH, 0.0);',
        'obs_data_set_default_double(settings, P_SPOT_BLUR_AMOUNT, 0.58);',
        'obs_data_set_default_double(settings, P_SPOT_BLUR_RADIUS, 3.8);',
        'obs_data_set_default_int(settings, P_SPOT_BLEND_MODE, 0);',
    ]

    for line in required:
        if line not in source:
            print(f"FAIL: missing spot default: {line}")
            return 1

    print("PASS: spot defaults preserve a small clear center with a physical diffusion ring")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
