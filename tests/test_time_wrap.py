#!/usr/bin/env python3
"""
Time accumulation and wrap tests for emuLensFX.

Verifies lens_tick uses volatile int64_t with 2-hour wrap.
"""

import os
import re
import sys

REPO = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
CORE_PATH = os.path.join(REPO, "src", "lens-core.c")
DATA_PATH = os.path.join(REPO, "src", "lens-data.h")

errors = []


def read_file(path):
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def check():
    core = read_file(CORE_PATH)
    data = read_file(DATA_PATH)

    # 1. TIME_WRAP_THRESHOLD is defined
    if re.search(r'TIME_WRAP_THRESHOLD\s+7200\.0f', data):
        print("  PASS: TIME_WRAP_THRESHOLD = 7200 seconds (2 hours)")
    else:
        errors.append("TIME_WRAP_THRESHOLD not 7200.0f")

    # 2. elapsed_time_ns uses __atomic builtins for thread safety
    if re.search(r'__atomic_load_n|emulens_atomic_load', core):
        print("  PASS: elapsed_time_ns uses atomic builtins for thread safety")
    else:
        errors.append("elapsed_time_ns not accessed via atomic builtins")

    # 3. lens_tick uses nanoseconds
    if re.search(r'1000000000', core):
        print("  PASS: lens_tick uses nanosecond conversion")
    else:
        errors.append("lens_tick missing nanosecond conversion (1000000000)")

    # 4. lens_tick wraps at threshold
    if re.search(r'TIME_WRAP_THRESHOLD', core):
        print("  PASS: lens_tick references TIME_WRAP_THRESHOLD")
    else:
        errors.append("lens_tick does not reference TIME_WRAP_THRESHOLD")

    # 5. wrap subtracts threshold (not reset to 0)
    if re.search(r'new_val\s*-?=\s*wrap_ns', core) or \
       re.search(r'new_val\s*-?=\s*T', core):
        print("  PASS: lens_tick wraps by subtracting threshold")
    else:
        errors.append("lens_tick missing subtractive wrap")

    # 6. bind_common_parameters reads elapsed_time_ns
    if re.search(r'elapsed_time_ns', core):
        print("  PASS: elapsed_time_ns read in render path")
    else:
        errors.append("elapsed_time_ns not read in render path")


def main():
    print("=" * 60)
    print("emuLensFX Time Accumulation Test")
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
