"""Regression contract for the Alpha-Piscium-inspired bloom pyramid.

This is intentionally structural: it protects the host-side render-target
orchestration that makes the bloom a real multiresolution pipeline instead of
quietly regressing to several full-resolution taps in one shader.
"""

from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CORE = (ROOT / "src" / "lens-core.c").read_text(encoding="utf-8")
DATA = (ROOT / "src" / "lens-data.h").read_text(encoding="utf-8")
BLOOM = (ROOT / "src" / "strategies" / "strategy-bloom.c").read_text(encoding="utf-8")
EFFECT = (ROOT / "data" / "emulens.effect").read_text(encoding="utf-8")


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise AssertionError(f"missing {label}: {needle}")


def main() -> int:
    require(DATA, "#define BLOOM_PYRAMID_LEVELS 4", "fixed pyramid level count")
    require(DATA, "bloom_pyramid[BLOOM_PYRAMID_LEVELS]", "downsample targets")
    require(DATA, "bloom_upsample[BLOOM_PYRAMID_LEVELS - 1]", "upsample targets")

    require(CORE, "gs_texrender_create(GS_RGBA16F, GS_ZS_NONE)", "FP16 bloom targets")
    require(CORE, "create_bloom_pyramid_buffers(filter)", "pyramid allocation")
    require(CORE, "destroy_bloom_pyramid_buffers(filter)", "pyramid teardown")
    require(CORE, '"DrawBloomDownsample"', "downsample technique cache")
    require(CORE, '"DrawBloomUpsample"', "upsample technique cache")

    require(BLOOM, "bloom_build_pyramid", "host-side pyramid orchestration")
    require(BLOOM, "bloom_half_dim", "resolution reduction")
    require(BLOOM, "filter->bloom_pyramid[i]", "downsample render target use")
    require(BLOOM, "filter->bloom_upsample[i]", "upsample render target use")
    require(BLOOM, "falling back to separable Gaussian blur", "graceful fallback")

    require(EFFECT, "BloomWeightedTap", "Karis-weighted taps")
    require(EFFECT, "PSBloomDownsample", "downsample shader")
    require(EFFECT, "PSBloomUpsample", "upsample shader")
    require(EFFECT, "technique DrawBloomDownsample", "downsample technique")
    require(EFFECT, "technique DrawBloomUpsample", "upsample technique")

    print("PASS: multiresolution bloom pyramid contract is intact")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
