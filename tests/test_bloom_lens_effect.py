#!/usr/bin/env python3

from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
EFFECT = (ROOT / "data/emulens.effect").read_text()
STRATEGY = (ROOT / "src/strategies/strategy-bloom.c").read_text()


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise AssertionError(f"missing {label}: {needle}")


def forbid(text: str, needle: str, label: str) -> None:
    if needle in text:
        raise AssertionError(f"forbidden {label}: {needle}")


def main() -> int:
    require(EFFECT, "uniform float bloom_dust_amount", "bloom dust amount uniform")
    require(EFFECT, "uniform float bloom_refract_amount", "bloom refraction amount uniform")
    require(EFFECT, "float BloomLensDirtPattern(float2 uv)", "fixed lens-dirt helper")

    if STRATEGY.count("rebind_current_settings(filter);") < 5:
        raise AssertionError(
            "Bloom multipass path must rebind mode settings after every common bind"
        )
    combine_bind = STRATEGY.index(
        "bind_common_parameters(filter, source_tex, width, height);",
        STRATEGY.index("// Final combine pass")
    )
    combine_rebind = STRATEGY.index("rebind_current_settings(filter);", combine_bind)
    combine_texture = STRATEGY.index(
        "SET_PARAM_TEXTURE(filter->common.blurred_image, blurred);",
        combine_rebind
    )
    if not combine_bind < combine_rebind < combine_texture:
        raise AssertionError(
            "Bloom combine must rebind settings before binding blurred_image"
        )

    helper_start = EFFECT.index("float BloomLensDirtPattern(float2 uv)")
    combine_start = EFFECT.index("float4 PSBloomCombine(")
    combine_end = EFFECT.index("technique DrawBloomExtract")
    helper = EFFECT[helper_start:combine_start]
    combine = EFFECT[combine_start:combine_end]

    # Dirt belongs to the lens surface; it should not drift like particles.
    forbid(helper, "elapsed_time", "time-drifting lens contamination")

    # Dust/haze/refraction are responses to the captured bright-pass energy.
    require(combine, "float light_energy =", "captured light energy")
    require(combine, "float light_gate = saturate(light_energy * 2.0);",
            "highlight-gated optical response")
    require(combine, "float dust_scatter =", "light-driven dust scatter")
    require(combine, "final_rgb += incident_light * dust_scatter", "bloom-driven dirt visibility")
    require(combine, "float glint =", "specular contamination glint")
    require(combine, "float haze_mask = saturate(", "light-driven veiling glare")
    require(combine, "float2 dirt_gradient =", "contamination surface gradient")
    require(combine, "float refract_response =", "light-gated refraction response")

    # Protect against the old painted-on dirt/fog regressions.
    forbid(combine, "float3 dust_visible_rgb", "source-independent dust overlay")
    forbid(combine, "(0.6 + glint)", "always-visible dust bias")
    forbid(combine, "final_rgb += bloom_color.rgb * haze_mask;",
           "source-independent haze overlay")

    # Extra optical toys remain supported rather than being removed.
    require(combine, "bloom_ghost_amount", "ghost reflections")
    require(combine, "bloom_anamorphic", "anamorphic optical path")

    print("PASS: Bloom lens contamination is fixed in lens space and revealed by light")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
