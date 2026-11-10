#!/usr/bin/env python3

import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent


RANGES = {
    "handheld_run.json": {
        "hand_pos_amp": (0.0, 0.1),
        "hand_pos_speed": (0.0, 5.0),
        "hand_rot_amp": (0.0, 10.0),
        "hand_rot_speed": (0.0, 10.0),
        "hand_zoom_amp": (0.0, 0.5),
        "hand_zoom_speed": (0.0, 10.0),
        "hand_blur_amp": (0.0, 20.0),
        "hand_blur_speed": (0.0, 10.0),
        "hand_blur_static": (0.0, 20.0),
        "hand_vel_dir_x": (-1.0, 1.0),
        "hand_vel_dir_y": (-1.0, 1.0),
        "hand_vel_mag": (0.0, 1.0),
        "hand_dir_blur_enable": (0, 1),
        "hand_blur_directional": (0, 1),
        "hand_feather": (0.0, 1.0),
    },
    "handheld_breathing.json": {
        "hand_pos_amp": (0.0, 0.1),
        "hand_rot_amp": (0.0, 10.0),
        "hand_zoom_amp": (0.0, 0.5),
        "hand_zoom_speed": (0.0, 10.0),
        "hand_feather": (0.0, 1.0),
    },
    "handheld_drift.json": {
        "hand_pos_amp": (0.0, 0.1),
        "hand_pos_speed": (0.0, 5.0),
        "hand_rot_amp": (0.0, 10.0),
        "hand_rot_speed": (0.0, 10.0),
        "hand_zoom_amp": (0.0, 0.5),
        "hand_zoom_speed": (0.0, 10.0),
        "hand_blur_amp": (0.0, 20.0),
        "hand_blur_speed": (0.0, 10.0),
        "hand_blur_static": (0.0, 20.0),
        "hand_vel_dir_x": (-1.0, 1.0),
        "hand_vel_dir_y": (-1.0, 1.0),
        "hand_vel_mag": (0.0, 1.0),
        "hand_dir_blur_enable": (0, 1),
        "hand_blur_directional": (0, 1),
        "hand_feather": (0.0, 1.0),
    },
    "bloom_cyberpunk.json": {
        "bloom_threshold": (0.0, 1.0),
        "bloom_intensity": (0.0, 10.0),
        "bloom_radius": (0.0, 100.0),
        "bloom_softness": (0.0, 1.0),
        "bloom_dust_amount": (0.0, 2.0),
        "bloom_dust_scale": (1.0, 50.0),
        "bloom_haze_amount": (0.0, 2.0),
        "bloom_dust_contrast": (0.0, 2.0),
        "bloom_refract_amount": (0.0, 1.0),
        "bloom_refract_scale": (0.1, 10.0),
    },
    "bloom_soft.json": {
        "bloom_threshold": (0.0, 1.0),
        "bloom_intensity": (0.0, 10.0),
        "bloom_radius": (0.0, 100.0),
        "bloom_softness": (0.0, 1.0),
        "bloom_dust_amount": (0.0, 2.0),
        "bloom_dust_scale": (1.0, 50.0),
        "bloom_haze_amount": (0.0, 2.0),
        "bloom_dust_contrast": (0.0, 2.0),
        "bloom_refract_amount": (0.0, 1.0),
        "bloom_refract_scale": (0.1, 10.0),
    },
    "bloom_prism.json": {
        "bloom_threshold": (0.0, 1.0),
        "bloom_intensity": (0.0, 10.0),
        "bloom_radius": (0.0, 100.0),
        "bloom_softness": (0.0, 1.0),
        "bloom_dust_amount": (0.0, 2.0),
        "bloom_dust_scale": (1.0, 50.0),
        "bloom_haze_amount": (0.0, 2.0),
        "bloom_dust_contrast": (0.0, 2.0),
        "bloom_refract_amount": (0.0, 1.0),
        "bloom_refract_scale": (0.1, 10.0),
    },
    "vidicon_soft.json": {
        "vid_decay": (0.0, 1.0),
        "vid_threshold": (0.0, 1.0),
        "vid_blend": (0.0, 1.0),
        "vid_red_lag": (0.0, 1.0),
        "vid_blue_lag": (0.0, 1.0),
        "vid_lag_nonlinear": (0.0, 1.0),
        "vid_beam_pull": (0.0, 1.0),
    },
    "starburst_cinema.json": {
        "sb_threshold": (0.0, 1.0),
        "sb_intensity": (0.0, 10.0),
        "sb_points": (2, 16),
        "sb_ray_length": (0.01, 1.0),
        "sb_thickness": (0.1, 10.0),
        "sb_smoothness": (0.1, 10.0),
        "sb_rot_speed": (-10.0, 10.0),
        "sb_rotation": (0.0, 360.0),
        "sb_pulse_speed": (0.0, 10.0),
        "sb_pulse_amp": (0.0, 1.0),
        "sb_samples": (4, 16),
    },
    "leak_amber.json": {
        "leak_intensity": (0.0, 5.0),
        "leak_speed": (0.0, 5.0),
        "leak_color_speed": (0.0, 5.0),
        "leak_scale": (0.1, 20.0),
        "leak_streakiness": (0.1, 20.0),
        "leak_complexity": (1, 10),
        "leak_bias_top": (0.0, 5.0),
        "leak_edge_falloff": (0.1, 20.0),
        "leak_contrast": (0.1, 10.0),
    },
    "bokeh_neon.json": {
        "bokeh_radius": (0.0, 50.0),
        "bokeh_power": (0.1, 10.0),
        "bokeh_shape": (0, 8),
        "bokeh_focus_x": (0.0, 1.0),
        "bokeh_focus_y": (0.0, 1.0),
        "bokeh_width": (0.01, 1.0),
        "bokeh_falloff": (0.001, 1.0),
        "bokeh_boost": (0.0, 50.0),
        "bokeh_threshold": (0.0, 1.0),
    },
    "part_snow.json": {
        "part_density": (1.0, 100.0),
        "part_size": (0.001, 0.2),
        "part_wind_x": (-1.0, 1.0),
        "part_wind_y": (-1.0, 1.0),
        "part_wobble": (0.0, 1.0),
        "part_focus_x": (0.0, 1.0),
        "part_focus_y": (0.0, 1.0),
        "part_focus_strength": (0.0, 1.0),
        "part_blur_edge": (0.0, 1.0),
        "part_poly_sides": (3, 10),
        "part_poly_rot": (0.0, 360.0),
        "part_poly_rot_speed": (-10.0, 10.0),
        "part_ca_strength": (0.0, 1.0),
    },
    "orton_velvet.json": {
        "orton_spread": (0.0, 50.0),
        "orton_opacity": (0.0, 1.0),
        "orton_contrast": (0.1, 5.0),
        "orton_brightness": (-1.0, 1.0),
        "orton_saturation": (0.0, 5.0),
        "orton_highlight_boost": (0.0, 5.0),
    },
    "grit_projector.json": {
        "grit_dust": (0.0, 1.0),
        "grit_scratches": (0.0, 1.0),
        "grit_noise": (0.0, 1.0),
        "grit_flares": (0.0, 1.0),
        "grit_brightness": (0.0, 1.0),
        "grit_contrast": (0.0, 5.0),
        "grit_saturation": (0.0, 5.0),
        "grit_opacity": (0.0, 1.0),
        "grit_instability": (0.0, 2.0),
    },
    "mirage_split.json": {
        "mir_count": (3, 12),
        "mir_radius": (0.0, 1.0),
        "mir_rotation": (0.0, 360.0),
        "mir_rot_speed": (-10.0, 10.0),
        "mir_center_mask": (0.0, 1.0),
    },
    "mirage_kaleido.json": {
        "mir_count": (3, 12),
        "mir_radius": (0.0, 1.0),
        "mir_rotation": (0.0, 360.0),
        "mir_rot_speed": (-10.0, 10.0),
        "mir_center_mask": (0.0, 1.0),
    },
    "mirage_drunk.json": {
        "mir_count": (3, 12),
        "mir_radius": (0.0, 1.0),
        "mir_rotation": (0.0, 360.0),
        "mir_rot_speed": (-10.0, 10.0),
        "mir_center_mask": (0.0, 1.0),
    },
    "spot_tunnel.json": {
        "spot_radius": (0.0, 1.0),
        "spot_softness": (0.0, 1.0),
        "spot_zoom_strength": (0.0, 1.0),
        "spot_blur_amount": (0.0, 1.0),
        "spot_blur_radius": (0.0, 20.0),
        "spot_blend_mode": (0, 2),
    },
    "spot_vignette.json": {
        "spot_radius": (0.0, 1.0),
        "spot_softness": (0.0, 1.0),
        "spot_blur_amount": (0.0, 1.0),
        "spot_blur_radius": (0.0, 20.0),
        "spot_blend_mode": (0, 2),
    },
    "spot_mist.json": {
        "spot_radius": (0.0, 1.0),
        "spot_softness": (0.0, 1.0),
        "spot_zoom_strength": (0.0, 1.0),
        "spot_blur_amount": (0.0, 1.0),
        "spot_blur_radius": (0.0, 20.0),
        "spot_blend_mode": (0, 2),
    },
}


def fail(message: str) -> int:
    print(f"FAIL: {message}")
    return 1


def main() -> int:
    for filename, checks in RANGES.items():
      data = json.loads((ROOT / "data/presets" / filename).read_text())
      for key, (min_val, max_val) in checks.items():
        if key not in data:
          return fail(f"{filename} missing {key}")
        value = data[key]
        if not (min_val <= value <= max_val):
          return fail(f"{filename} {key} out of range: {value}")

    print("PASS: shipped presets stay within intended ranges")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
