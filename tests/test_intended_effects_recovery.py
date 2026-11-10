#!/usr/bin/env python3

from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EFFECT = (ROOT / "data" / "emulens.effect").read_text()
UI = (ROOT / "src" / "lens-ui.c").read_text()


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise AssertionError(f"missing {label}: {needle}")


def forbid(text: str, needle: str, label: str) -> None:
    if needle in text:
        raise AssertionError(f"forbidden {label}: {needle}")


def main() -> int:
    require(EFFECT, "float size_energy = sqrt(min(1.0, 0.004 / max(size, 0.004)));",
            "particle size-energy compensation")
    require(EFFECT, "float3 particle_layer = saturate(accum);",
            "bounded particle layer")
    forbid(EFFECT, "float4 result = float4(original.rgb + accum, original.a);",
           "old additive particle cloud")

    require(EFFECT, "float3 BloomSampleImageBlack(float2 uv)",
            "black-border bloom image sampler")
    require(EFFECT, "float3 BloomSampleBlurredBlack(float2 uv)",
            "black-border bloom pyramid sampler")
    require(EFFECT, "small += BloomSampleBlurredBlack(",
            "black-border bloom reconstruction")

    require(EFFECT, "float reach = pow(excess, 0.60);",
            "highlight-driven star reach")
    forbid(EFFECT, "float gate = sb_threshold *",
           "distance-lowered star threshold")

    require(UI, 'obs_module_text("SpotOpticType")',
            "center-spot optical selector")
    require(EFFECT, "if (optic == 0)", "soft-spot optical branch")
    require(EFFECT, "} else if (optic == 1) {", "diopter optical branch")
    require(EFFECT, "PSCenterSpotRadialZoom", "radial-zoom optical branch")
    forbid(EFFECT, "edge_rgb = original.rgb * spot_rgb;",
           "old multiply pseudo-optic")

    vidicon_c = (ROOT / "src" / "strategies" / "strategy-vidicon.c").read_text()
    require(vidicon_c, "vidicon_decay_half_life_seconds",
            "perceptual Vidicon decay mapping")
    require(vidicon_c, "vidicon_nominal_60hz_decay",
            "half-life to frame-retention conversion")
    require(vidicon_c, "powf(nominal_60hz_decay, dt * 60.0f)",
            "time-normalized Vidicon decay")
    forbid(vidicon_c, "user_decay * 0.995f + 0.004f",
           "raw Vidicon coefficient mapping")
    require(vidicon_c, "dt = clamp_float(dt, 0.0f, 0.25f)",
            "bounded Vidicon elapsed-time step")

    require(EFFECT, "vid_red_lag * (1.0 - base_decay) * 1.00",
            "red temporal retention")
    require(EFFECT, "vid_blue_lag * (1.0 - base_decay) * 0.65",
            "blue temporal retention")
    require(EFFECT, "float4 history = history_image.Sample(textureSampler, history_uv);",
            "shared temporal history sample")
    forbid(EFFECT, "float2 lag_r =", "spatial red lag offset")
    forbid(EFFECT, "float2 lag_b =", "spatial blue lag offset")
    forbid(EFFECT, "lerp(1.0, 0.55", "brightness-forced faster discharge")

    print("PASS: intended particle, bloom, starburst, center-spot, and Vidicon mechanisms are protected")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
