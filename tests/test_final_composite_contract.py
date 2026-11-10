#!/usr/bin/env python3
from pathlib import Path
import sys
s=(Path(__file__).resolve().parents[1]/"src/lens-core.c").read_text()
start=s.index("gs_texture_t *composite_tex")
end=s.index("pthread_mutex_unlock(&filter->render_mutex)", start)
body=s[start:end]
errors=[]
for token in (
    "obs_get_base_effect(OBS_EFFECT_DEFAULT)",
    'gs_effect_get_technique(copy_effect, "Draw")',
    'gs_effect_get_param_by_name(copy_effect, "image")',
    "gs_enable_framebuffer_srgb(linear_srgb)",
    "gs_effect_set_texture_srgb(image_param, composite_tex)",
    "gs_effect_set_texture(image_param, composite_tex)",
    "gs_draw_sprite(composite_tex, 0, target_w, target_h)",
):
    if token not in body:
        errors.append(f"missing {token}")
if "filter->tech.draw" in body:
    errors.append("custom EmuLens Draw technique reused for final composite")
if errors:
    for e in errors: print("FAIL:", e)
    sys.exit(1)
print("PASS: final composite uses OBS base Draw effect")
