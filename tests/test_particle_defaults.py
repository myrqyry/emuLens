#!/usr/bin/env python3

from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent


def main() -> int:
    source = (ROOT / "src/strategies/strategy-particles.c").read_text()

    required = [
        'obs_data_set_default_double(settings, P_PART_DENSITY, 18.0);',
        'obs_data_set_default_double(settings, P_PART_SIZE, 0.004);',
        'obs_data_set_default_double(settings, P_PART_SIZE_VAR, 0.55);',
        'obs_data_set_default_double(settings, P_PART_SPEED, 0.25);',
        'obs_data_set_default_bool(settings, P_PART_SOURCE_BRIGHTNESS, false);',
        'obs_data_set_default_double(settings, P_PART_SOURCE_THRESHOLD, 0.70);',
        'obs_data_set_default_double(settings, P_PART_SOURCE_STRENGTH, 1.0);',
        'obs_data_set_default_double(settings, P_PART_FOCUS_STRENGTH, 0.0);',
        'obs_data_set_default_double(settings, P_PART_BLUR_EDGE, 0.65);',
    ]

    for line in required:
        if line not in source:
            print(f"FAIL: missing particle default: {line}")
            return 1

    print("PASS: particle defaults stay sparse and translucent")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
