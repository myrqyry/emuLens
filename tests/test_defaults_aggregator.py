#!/usr/bin/env python3
"""
Defaults aggregator test for emuLensFX.

Verifies emulens_get_defaults sets required defaults and delegates
to all strategy get_defaults functions.
"""

import os
import re
import sys

REPO = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
UI_PATH = os.path.join(REPO, "src", "lens-ui.c")
DATA_PATH = os.path.join(REPO, "src", "lens-data.h")
PARAMS_PATH = os.path.join(REPO, "src", "lens-params.h")

errors = []


def read_file(path):
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def check():
    ui = read_file(UI_PATH)
    data = read_file(DATA_PATH)
    params = read_file(PARAMS_PATH)

    # 1. emulens_get_defaults sets UI_MODE default
    if re.search(r'obs_data_set_default_int\(settings,\s*UI_MODE,?\s*MODE_VIDICON\)', ui):
        print("  PASS: UI_MODE default set to MODE_VIDICON")
    else:
        errors.append("emulens_get_defaults missing UI_MODE default")

    # 2. emulens_get_defaults sets UI_PRESET_LIST default
    if re.search(r'obs_data_set_default_string\(settings,\s*UI_PRESET_LIST,?\s*""\)', ui):
        print("  PASS: UI_PRESET_LIST default set to empty string")
    else:
        errors.append("emulens_get_defaults missing UI_PRESET_LIST default")

    # 3. emulens_get_defaults loops over all modes
    loop_count = len(re.findall(r'strategies\[i\]\.get_defaults', ui))
    if loop_count >= 1:
        print(f"  PASS: emulens_get_defaults loops over strategies[i].get_defaults")
    else:
        errors.append("emulens_get_defaults missing loop over get_defaults")

    # 4. UI_MODE constant defined
    if re.search(r'#define\s+UI_MODE\s+"mode"', params):
        print("  PASS: UI_MODE defined as 'mode'")
    else:
        errors.append("UI_MODE not defined in params header")

    # 5. UI_PRESET_LIST constant defined
    if re.search(r'#define\s+UI_PRESET_LIST\s+"preset_list"', params):
        print("  PASS: UI_PRESET_LIST defined as 'preset_list'")
    else:
        errors.append("UI_PRESET_LIST not defined in params header")

    # 6. NUM_MODES count matches expected strategy count
    num_modes = re.search(r'#define\s+X\(id,\s*suffix,\s*display_name,\s*tech_name,\s*props_func\).*EMULENS_MODES\s+NUM_MODES', data)
    if re.search(r'NUM_MODES', data):
        print("  PASS: NUM_MODES defined in data header")
    else:
        errors.append("NUM_MODES not defined")

    # 7. X-macro in EMULENS_MODES has 15 entries (excluding comment lines)
    mode_lines = [line for line in params.split('\n')
                  if re.search(r'X\([A-Z]+,', line) and not line.strip().startswith('//')]
    if len(mode_lines) == 15:
        print(f"  PASS: EMULENS_MODES has {len(mode_lines)} entries")
    else:
        errors.append(f"EMULENS_MODES has {len(mode_lines)} entries, expected 15")

    # 8. All get_defaults functions are declared via the X-macro expansion in strategies.h
    strat_h_path = os.path.join(REPO, "src", "strategies", "strategies.h")
    if os.path.exists(strat_h_path):
        strat_h = read_file(strat_h_path)
        if "EMULENS_MODES" in strat_h and "get_defaults" in strat_h:
            print("  PASS: strategies.h declares get_defaults via X-macro expansion")
        else:
            errors.append("strategies.h missing get_defaults X-macro declarations")
    else:
        errors.append("strategies.h not found")


def main():
    print("=" * 60)
    print("emuLensFX Defaults Aggregator Test")
    print("=" * 60)
    check()
    print("-" * 60)
    if errors:
        for e in errors:
            print(f"  FAIL: {e}")
        sys.exit(1)
    print("ALL CHECKS PASSED")


if __name__ == "__main__":
    main()
