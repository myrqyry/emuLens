#!/usr/bin/env python3

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PRESETS = ROOT / "data/presets"


def load(name: str) -> dict:
    return json.loads((PRESETS / name).read_text(encoding="utf-8"))


def require(condition: bool, message: str, failures: list[str]) -> None:
    if not condition:
        failures.append(message)


def main() -> int:
    failures: list[str] = []

    defaults = sorted(PRESETS.glob("default_*.json"))
    require(len(defaults) == 17, "there must be exactly one default preset per mode", failures)
    require(
        {json.loads(p.read_text())["mode"] for p in defaults} == set(range(17)),
        "default presets must cover all 17 modes exactly once",
        failures,
    )

    vid = load("default_vidicon.json")
    require(0.75 <= vid["vid_decay"] <= 0.95, "default Vidicon persistence is outside the natural time-normalized tube range", failures)
    require(vid["vid_blend"] <= 0.10, "default Vidicon trail tint is too strong", failures)
    require(not vid["vid_comet"], "default Vidicon must not enable comet trails", failures)
    require(not vid["vid_smear_enable"], "default Vidicon must not enable vertical smear", failures)
    require(vid["vid_register"] <= 0.20, "default Vidicon registration error is too large", failures)

    star = load("default_starburst.json")
    require(star["sb_intensity"] <= 1.0, "default Starburst is overbright", failures)
    require(star["sb_rot_speed"] == 0.0, "default Starburst must be static", failures)
    require(star["sb_pulse_amp"] == 0.0, "default Starburst must not pulse", failures)

    hand = load("default_handheld.json")
    require(not hand["hand_blur_enable"], "default Handheld must not force motion blur", failures)
    require(hand["hand_step_amp"] == 0.0, "default Handheld must not simulate walking", failures)

    bloom = load("default_bloom.json")
    require(bloom["bloom_intensity"] <= 1.0, "default Bloom intensity exceeds a restrained baseline", failures)
    require(bloom["bloom_dust_amount"] <= 0.05, "default Bloom lens dirt is too visible", failures)
    require(bloom["bloom_haze_amount"] <= 0.05, "default Bloom haze is too visible", failures)
    require(bloom["bloom_refract_amount"] == 0.0, "default Bloom must not add refraction", failures)
    require(bloom["bloom_ghost_amount"] == 0.0, "default Bloom must not add lens ghosts", failures)

    grit = load("default_grit.json")
    require(grit["grit_opacity"] <= 0.40, "default Grit opacity is too high", failures)
    require(grit["grit_instability"] <= 0.20, "default Grit gate weave is too strong", failures)

    pinhole = load("default_pinhole.json")
    require(pinhole["pinhole_sepia"] <= 0.10, "default Pinhole must not be a sepia preset", failures)
    require("pinhole_distortion" not in pinhole, "retired Pinhole distortion leaked into curated presets", failures)

    tech = load("default_technicolor.json")
    require(tech["tech_saturation"] <= 1.20, "default Technicolor saturation is too high", failures)
    require(tech["tech_offset"] <= 0.05, "default Technicolor registration error is too large", failures)

    dag = load("default_daguerre.json")
    require(dag["dag_contrast"] <= 0.60, "default Daguerreotype clips too much after shader contrast remap", failures)

    # Named presets get personality, but they still obey a hierarchy instead of
    # maxing every control at once.
    for name in ("bloom_soft.json", "bloom_cyberpunk.json", "bloom_prism.json"):
        p = load(name)
        require(p["bloom_intensity"] <= 1.10, f"{name}: bloom intensity is demo-reel territory", failures)
        require(p["bloom_dust_amount"] <= 0.08, f"{name}: dust overwhelms the optical effect", failures)
        require(p["bloom_haze_amount"] <= 0.08, f"{name}: haze overwhelms the optical effect", failures)
        require(p["bloom_refract_amount"] <= 0.08, f"{name}: refraction is too dominant", failures)
        require(p["bloom_ghost_amount"] <= 0.12, f"{name}: ghost reflections are too dominant", failures)

    dusty = load("bloom_dusty_lens.json")
    require(0.20 <= dusty["bloom_dust_amount"] <= 0.60,
            "bloom_dusty_lens.json: lens contamination should be clearly visible when lit", failures)
    require(0.05 <= dusty["bloom_haze_amount"] <= 0.25,
            "bloom_dusty_lens.json: veiling glare should support rather than wash out the image", failures)
    require(dusty["bloom_refract_amount"] <= 0.10,
            "bloom_dusty_lens.json: refraction should remain subordinate to scatter", failures)
    require(dusty["bloom_ghost_amount"] <= 0.10,
            "bloom_dusty_lens.json: ghosts should remain a supporting artifact", failures)

    phone = load("default_phone_wet.json")
    require(phone["phone_ribbons"] <= 0.55,
            "phone wet default must avoid strong night-street ribbons", failures)
    require(phone["phone_arcs"] <= 0.5,
            "phone wet default must avoid overbright wet contours", failures)
    require(phone["phone_glow"] <= 0.35,
            "phone wet default must preserve original source visibility", failures)
    street = load("phone_wet_streetlights.json")
    require(street["phone_ribbons"] > phone["phone_ribbons"],
            "streetlight preset must meaningfully extend source lights", failures)
    require(street["phone_reach"] > phone["phone_reach"],
            "streetlight preset must meaningfully extend the smear", failures)

    for name in ("orton_angelic.json", "orton_heavy.json", "orton_velvet.json"):
        p = load(name)
        require(p["orton_opacity"] <= 0.45, f"{name}: Orton blend is too heavy", failures)
        require(-0.10 <= p["orton_brightness"] <= 0.05, f"{name}: Orton brightness compensation is implausible", failures)

    soft = load("vidicon_soft.json")
    dream = load("vidicon_dream.json")
    require(0.55 <= soft["vid_decay"] <= 0.70, "vidicon_soft.json: clean-tube persistence is outside the short perceptual range", failures)
    require(0.90 <= dream["vid_decay"] <= 0.99, "vidicon_dream.json: long-exposure persistence should be long but bounded", failures)
    for name, p in (("vidicon_soft.json", soft), ("vidicon_dream.json", dream)):
        require(p["vid_register"] <= 0.20, f"{name}: Vidicon registration error is too strong", failures)

    for name in ("technicolor_classic.json", "technicolor_misaligned.json"):
        require(load(name)["tech_saturation"] <= 1.25, f"{name}: saturation is overcooked", failures)

    for name in ("default_leak.json", "leak_warm.json", "leak_amber.json", "leak_chaos.json"):
        require(load(name)["leak_intensity"] <= 1.25, f"{name}: light leak intensity is overcooked", failures)

    for name in ("default_bokeh.json", "bokeh_tiltshift.json", "bokeh_dreamy.json", "bokeh_neon.json"):
        p = load(name)
        require(p["bokeh_radius"] <= 14.0, f"{name}: bokeh blur is too destructive", failures)
        require(p["bokeh_boost"] <= 7.0, f"{name}: bokeh highlights are too aggressively weighted", failures)

    for path in PRESETS.glob("spot_*.json"):
        require("spot_tint" not in load(path.name), f"{path.name}: retired Spot tint leaked into curated presets", failures)

    if failures:
        print("FAIL: preset restraint contract")
        for failure in failures:
            print(f"  - {failure}")
        return 1

    print("PASS: defaults and named presets preserve visual hierarchy")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
