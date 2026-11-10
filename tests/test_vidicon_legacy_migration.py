#!/usr/bin/env python3

from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CORE = (ROOT / "src" / "lens-core.c").read_text()


def require(term: str) -> None:
    if term not in CORE:
        raise AssertionError(f"missing Vidicon migration contract: {term}")


def main() -> int:
    require("static bool vidicon_needs_legacy_migration(obs_data_t *settings)")
    require("obs_data_has_user_value(settings, P_VID_RED_LAG)")
    require("obs_data_has_user_value(settings, P_VID_BLUE_LAG)")
    require("obs_data_get_double(settings, P_VID_THRESHOLD) >= 0.995")
    require("return legacy_score >= 4;")
    require('obs_data_set_double(settings, P_VID_THRESHOLD, 0.64);')
    require('obs_data_set_double(settings, P_VID_RED_LAG, 0.28);')
    require('obs_data_set_double(settings, P_VID_BLUE_LAG, 0.10);')
    require('obs_data_set_bool(settings, P_VID_SOURCE_COLOR, true);')
    require("if (requested_mode == MODE_VIDICON)")
    require("migrate_legacy_vidicon_settings(settings);")

    migrate_pos = CORE.index("migrate_legacy_vidicon_settings(settings);")
    copy_pos = CORE.index("obs_data_apply(settings_copy, settings);")
    if migrate_pos > copy_pos:
        raise AssertionError("legacy settings migration must happen before settings snapshot")

    print("PASS: dead churn-era Vidicon settings migrate before render snapshot")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
