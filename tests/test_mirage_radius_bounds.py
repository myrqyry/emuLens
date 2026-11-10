#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EFFECT = (ROOT / "data" / "emulens.effect").read_text()

def main() -> int:
    for item in [
        "float2 MirageMirrorUV(float2 uv)",
        "float2 folded = frac(uv * 0.5) * 2.0;",
        "return 1.0 - abs(folded - 1.0);",
        "float2 samp_uv = MirageMirrorUV(v_in.uv + offset);",
        "float sample_count = max(1.0, float(count));",
    ]:
        if item not in EFFECT:
            raise AssertionError(f"missing Mirage large-radius contract: {item}")
    for item in [
        "int valid_samples = 0;",
        "valid_samples++;",
        "float sample_count = max(1.0, float(valid_samples));",
    ]:
        if item in EFFECT:
            raise AssertionError(f"stale Mirage out-of-bounds drop path remains: {item}")
    print("PASS: Mirage large-radius samples mirror at image bounds instead of collapsing to black")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
