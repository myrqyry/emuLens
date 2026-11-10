#!/usr/bin/env python3

import json
import math
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PARAMS = (ROOT / "src/lens-params.h").read_text(encoding="utf-8")

DEFAULTS = {
    "default_vidicon.json": ("strategy-vidicon.c", 0),
    "default_starburst.json": ("strategy-starburst.c", 1),
    "default_leak.json": ("strategy-leak.c", 2),
    "default_handheld.json": ("strategy-handheld.c", 3),
    "default_bokeh.json": ("strategy-bokeh.c", 4),
    "default_particles.json": ("strategy-particles.c", 5),
    "default_orton.json": ("strategy-orton.c", 6),
    "default_grit.json": ("strategy-grit.c", 7),
    "default_mirage.json": ("strategy-mirage.c", 8),
    "default_spot.json": ("strategy-spot.c", 9),
    "default_bloom.json": ("strategy-bloom.c", 10),
    "default_pinhole.json": ("strategy-pinhole.c", 11),
    "default_polaroid.json": ("strategy-polaroid.c", 12),
    "default_daguerre.json": ("strategy-daguerreotype.c", 13),
    "default_technicolor.json": ("strategy-technicolor.c", 14),
    "default_wet_lens.json": ("strategy-wet-lens.c", 15),
    "default_phone_wet.json": ("strategy-phone-wet.c", 16),
}

# These persisted settings intentionally remain wired for backward compatibility
# but are retired from the curated UI/presets because they have no visible
# behavior in the current shader.
RETIRED_KEYS = {"spot_tint", "pinhole_distortion"}


def parse_scalar(text: str, constants: dict[str, object]) -> object:
    token = text.strip()
    if token in constants:
        return constants[token]
    if token == "true":
        return True
    if token == "false":
        return False
    if token.lower().startswith("0x"):
        return int(token, 16)
    token = token.rstrip("fF")
    try:
        return int(token)
    except ValueError:
        return float(token)


def constants_from_header() -> dict[str, object]:
    constants: dict[str, object] = {}
    for name, value in re.findall(r"^#define\s+(\w+)\s+([^\s/]+)", PARAMS, re.M):
        try:
            constants[name] = parse_scalar(value, constants)
        except (ValueError, TypeError):
            pass
    return constants


def property_keys() -> dict[str, str]:
    return dict(re.findall(r'^#define\s+(P_\w+)\s+"([^"]+)"', PARAMS, re.M))


def function_body(source: str, name: str) -> str:
    marker = f"void {name}(obs_data_t *settings) {{"
    start = source.find(marker)
    if start < 0:
        raise AssertionError(f"missing {name}")
    brace = source.find("{", start)
    depth = 0
    for index in range(brace, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[brace + 1 : index]
    raise AssertionError(f"unterminated {name}")


def strategy_defaults(path: Path, constants: dict[str, object], keys: dict[str, str]) -> dict[str, object]:
    source = path.read_text(encoding="utf-8")
    strategy = path.stem.removeprefix("strategy-").replace("-", "_")
    body = function_body(source, f"{strategy}_get_defaults")
    values: dict[str, object] = {}
    pattern = re.compile(
        r"obs_data_set_default_(?:double|int|bool)\(\s*settings,\s*(P_\w+),\s*([^)]+?)\s*\);",
        re.S,
    )
    for param, value in pattern.findall(body):
        if param not in keys:
            raise AssertionError(f"{path.name}: unknown property macro {param}")
        values[keys[param]] = parse_scalar(value, constants)
    return values


def equal(left: object, right: object) -> bool:
    if isinstance(left, bool) or isinstance(right, bool):
        return left is right
    if isinstance(left, (int, float)) and isinstance(right, (int, float)):
        return math.isclose(float(left), float(right), rel_tol=0.0, abs_tol=1e-6)
    return left == right


def main() -> int:
    constants = constants_from_header()
    keys = property_keys()
    failures: list[str] = []

    preset_files = sorted((ROOT / "data/presets").glob("default_*.json"))
    if len(preset_files) != len(DEFAULTS):
        failures.append(
            f"expected {len(DEFAULTS)} default presets (one per mode), found {len(preset_files)}"
        )

    for filename, (strategy_file, expected_mode) in DEFAULTS.items():
        preset_path = ROOT / "data/presets" / filename
        if not preset_path.exists():
            failures.append(f"missing {filename}")
            continue

        preset = json.loads(preset_path.read_text(encoding="utf-8"))
        if preset.get("mode") != expected_mode:
            failures.append(
                f"{filename}: mode {preset.get('mode')} != expected {expected_mode}"
            )

        c_defaults = strategy_defaults(
            ROOT / "src/strategies" / strategy_file, constants, keys
        )
        visible_c = {k: v for k, v in c_defaults.items() if k not in RETIRED_KEYS}
        preset_values = {k: v for k, v in preset.items() if k != "mode"}

        missing = set(visible_c) - set(preset_values)
        extra = set(preset_values) - set(visible_c)
        if missing:
            failures.append(f"{filename}: missing C defaults {sorted(missing)}")
        if extra:
            failures.append(f"{filename}: preset-only keys {sorted(extra)}")

        for key in sorted(set(visible_c) & set(preset_values)):
            if not equal(visible_c[key], preset_values[key]):
                failures.append(
                    f"{filename}: {key}={preset_values[key]!r} != C default {visible_c[key]!r}"
                )

    if failures:
        print("FAIL: default preset contract drifted")
        for failure in failures:
            print(f"  - {failure}")
        return 1

    print("PASS: all 17 default presets match their visible strategy defaults")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
