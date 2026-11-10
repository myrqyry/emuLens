#!/usr/bin/env python3

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent


def read(path: Path) -> str:
    return path.read_text()


def expect(pattern: str, text: str, message: str) -> bool:
    if not re.search(pattern, text, re.S):
        print(f"FAIL: {message}")
        return False
    return True


def main() -> int:
    ui = read(ROOT / "src/lens-ui.c")
    starburst = read(ROOT / "src/strategies/strategy-starburst.c")
    handheld = read(ROOT / "src/strategies/strategy-handheld.c")
    leak = read(ROOT / "src/strategies/strategy-leak.c")
    particles = read(ROOT / "src/strategies/strategy-particles.c")
    bloom = read(ROOT / "src/strategies/strategy-bloom.c")
    mirage = read(ROOT / "src/strategies/strategy-mirage.c")

    checks = [
        (ui, r'P_SB_ROT_SPEED[^;]*?-3\.0,\s*3\.0', "starburst rotation speed UI range"),
        (starburst, r'P_SB_ROT_SPEED\b[^)]*?-\s*3\.0f\s*,\s*3\.0f', "starburst rotation speed clamp"),
        (ui, r'P_SB_PULSE_SPEED[^;]*?0\.0,\s*3\.0', "starburst pulse speed UI range"),
        (starburst, r'P_SB_PULSE_SPEED\b[^)]*?0\.0f\s*,\s*3\.0f', "starburst pulse speed clamp"),
        (ui, r'P_SB_SAMPLES[^;]*?4,\s*16', "starburst samples UI range"),
        (starburst, r'P_SB_SAMPLES\b[^)]*?4\.0f\s*,\s*16\.0f', "starburst samples clamp"),
        (ui, r'P_LEAK_INTENSITY[^;]*?0\.0,\s*5\.0', "leak intensity UI range"),
        (leak, r'P_LEAK_INTENSITY\b[^)]*?0\.0f\s*,\s*5\.0f', "leak intensity clamp"),
        (ui, r'P_LEAK_SPEED[^;]*?0\.0,\s*2\.0', "leak speed UI range"),
        (leak, r'P_LEAK_SPEED\b[^)]*?0\.0f\s*,\s*2\.0f', "leak speed clamp"),
        (ui, r'P_LEAK_SCALE[^;]*?0\.1,\s*20\.0', "leak scale UI range"),
        (leak, r'P_LEAK_SCALE\b[^)]*?0\.1f\s*,\s*20\.0f', "leak scale clamp"),
        (ui, r'P_LEAK_COLOR_SPEED[^;]*?0\.0,\s*2\.0', "leak color speed UI range"),
        (leak, r'P_LEAK_COLOR_SPEED\b[^)]*?0\.0f\s*,\s*2\.0f', "leak color speed clamp"),
        (ui, r'P_LEAK_BIAS_TOP[^;]*?0\.0,\s*5\.0', "leak bias top UI range"),
        (leak, r'P_LEAK_BIAS_TOP\b[^)]*?0\.0f\s*,\s*5\.0f', "leak bias top clamp"),
        (ui, r'P_LEAK_EDGE_FALLOFF[^;]*?0\.1,\s*20\.0', "leak edge falloff UI range"),
        (leak, r'P_LEAK_EDGE_FALLOFF\b[^)]*?0\.1f\s*,\s*20\.0f', "leak edge falloff clamp"),
        (ui, r'P_LEAK_STREAKINESS[^;]*?0\.1,\s*20\.0', "leak streakiness UI range"),
        (leak, r'P_LEAK_STREAKINESS\b[^)]*?0\.1f\s*,\s*20\.0f', "leak streakiness clamp"),
        (ui, r'P_LEAK_CONTRAST[^;]*?0\.1,\s*10\.0', "leak contrast UI range"),
        (leak, r'P_LEAK_CONTRAST\b[^)]*?0\.1f\s*,\s*10\.0f', "leak contrast clamp"),
        (ui, r'P_LEAK_COMPLEXITY[^;]*?1,\s*10', "leak complexity UI range"),
        (leak, r'P_LEAK_COMPLEXITY\b[^)]*?1\.0f\s*,\s*10\.0f', "leak complexity clamp"),
        (ui, r'P_HAND_ROT_SPEED[^;]*?0\.0,\s*5\.0', "handheld rotation speed UI range"),
        (handheld, r'P_HAND_ROT_SPEED\),\s*0\.0f,\s*5\.0f', "handheld rotation speed clamp"),
        (ui, r'P_HAND_ZOOM_AMP[^;]*?0\.0,\s*0\.5', "handheld zoom amp UI range"),
        (handheld, r'P_HAND_ZOOM_AMP\),\s*0\.0f,\s*0\.5f', "handheld zoom amp clamp"),
        (ui, r'P_HAND_ZOOM_SPEED[^;]*?0\.0,\s*5\.0', "handheld zoom speed UI range"),
        (handheld, r'P_HAND_ZOOM_SPEED\),\s*0\.0f,\s*5\.0f', "handheld zoom speed clamp"),
        (ui, r'P_HAND_BLUR_AMP[^;]*?0\.0,\s*20\.0', "handheld blur amp UI range"),
        (handheld, r'P_HAND_BLUR_AMP\),\s*0\.0f,\s*20\.0f', "handheld blur amp clamp"),
        (ui, r'P_HAND_BLUR_SPEED[^;]*?0\.0,\s*10\.0', "handheld blur speed UI range"),
        (handheld, r'P_HAND_BLUR_SPEED\),\s*0\.0f,\s*10\.0f', "handheld blur speed clamp"),
        (ui, r'P_PART_DENSITY[^;]*?1\.0,\s*40\.0', "particles density UI range"),
        (particles, r'P_PART_DENSITY\),\s*1\.0f,\s*40\.0f', "particles density clamp"),
        (ui, r'P_PART_SIZE[^;]*?0\.001,\s*0\.1', "particles size UI range"),
        (particles, r'P_PART_SIZE\),\s*0\.001f,\s*0\.1f', "particles size clamp"),
        (ui, r'P_PART_SOURCE_STRENGTH[^;]*?0\.0,\s*5\.0', "particles source strength UI range"),
        (particles, r'P_PART_SOURCE_STRENGTH\),\s*0\.0f,\s*5\.0f', "particles source strength clamp"),
        (ui, r'P_PART_POLY_ROT_SPEED[^;]*?-10\.0,\s*10\.0', "particles poly rotation speed UI range"),
        (particles, r'P_PART_POLY_ROT_SPEED\),\s*-10\.0f,\s*10\.0f', "particles poly rotation speed clamp"),
        (ui, r'P_MIR_ROT_SPEED[^;]*?-10\.0,\s*10\.0', "mirage rotation speed UI range"),
        (mirage, r'P_MIR_ROT_SPEED\b[^)]*?-\s*10\.0f\s*,\s*10\.0f', "mirage rotation speed clamp"),
        (ui, r'P_BLOOM_DUST_AMOUNT[^;]*?0\.0,\s*2\.0', "bloom dust amount UI range"),
        (bloom, r'P_BLOOM_DUST_AMOUNT\b[^)]*?0\.0f\s*,\s*2\.0f', "bloom dust amount clamp"),
        (ui, r'P_BLOOM_DUST_SCALE[^;]*?1\.0,\s*50\.0', "bloom dust scale UI range"),
        (bloom, r'P_BLOOM_DUST_SCALE\b[^)]*?1\.0f\s*,\s*50\.0f', "bloom dust scale clamp"),
        (ui, r'P_BLOOM_REFRACT_AMOUNT[^;]*?0\.0,\s*1\.0', "bloom refraction amount UI range"),
        (bloom, r'P_BLOOM_REFRACT_AMOUNT\b[^)]*?0\.0f\s*,\s*1\.0f', "bloom refraction amount clamp"),
        (ui, r'P_BLOOM_REFRACT_SCALE[^;]*?0\.1,\s*10\.0', "bloom refraction scale UI range"),
        (bloom, r'P_BLOOM_REFRACT_SCALE\b[^)]*?0\.1f\s*,\s*10\.0f', "bloom refraction scale clamp"),
    ]

    ok = True
    for text, pattern, message in checks:
        ok = expect(pattern, text, message) and ok

    if ok:
        print("PASS: customization ranges are aligned")
        return 0
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
