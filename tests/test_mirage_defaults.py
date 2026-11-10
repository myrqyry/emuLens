#!/usr/bin/env python3

from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent


def main() -> int:
    source = (ROOT / "src/strategies/strategy-mirage.c").read_text()

    required = [
        'obs_data_set_default_int(settings, P_MIR_COUNT, 4);',
        'obs_data_set_default_double(settings, P_MIR_RADIUS, 0.035);',
        'obs_data_set_default_double(settings, P_MIR_ROT_SPEED, 0.0);',
    ]

    for line in required:
        if line not in source:
            print(f"FAIL: missing mirage default: {line}")
            return 1

    print("PASS: mirage defaults keep the optical echo subtle and static")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
