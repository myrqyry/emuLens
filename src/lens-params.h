#pragma once

#include <graphics/graphics.h>
#include <obs-module.h>
#include <util/dstr.h>

// Vidicon parameters
#define VID_DECAY_MIN 0.0f
#define VID_DECAY_MAX 1.0f
#define VID_DECAY_DEFAULT 0.88f

// Starburst parameters
#define SB_THRESHOLD_MIN 0.0f
#define SB_THRESHOLD_MAX 1.0f
#define SB_THRESHOLD_DEFAULT 0.84f

#define SB_INTENSITY_MIN 0.0f
#define SB_INTENSITY_MAX 10.0f
#define SB_INTENSITY_DEFAULT 0.65f

// Handheld parameters
#define HAND_POS_AMP_MIN 0.0f
#define HAND_POS_AMP_MAX 0.1f
#define HAND_POS_AMP_DEFAULT 0.0035f

#define HAND_POS_SPEED_MIN 0.0f
#define HAND_POS_SPEED_MAX 3.0f
#define HAND_POS_SPEED_DEFAULT 0.16f

#define HAND_ROT_AMP_MIN 0.0f
#define HAND_ROT_AMP_MAX 10.0f
#define HAND_ROT_AMP_DEFAULT 0.22f

#define HAND_TREMOR_AMP_MIN 0.0f
#define HAND_TREMOR_AMP_MAX 1.0f
#define HAND_TREMOR_AMP_DEFAULT 0.025f

#define HAND_BREATH_AMP_MIN 0.0f
#define HAND_BREATH_AMP_MAX 1.0f
#define HAND_BREATH_AMP_DEFAULT 0.12f

#define HAND_STEP_AMP_MIN 0.0f
#define HAND_STEP_AMP_MAX 1.0f
#define HAND_STEP_AMP_DEFAULT 0.0f

#define HAND_STEP_SPEED_MIN 0.0f
#define HAND_STEP_SPEED_MAX 3.0f
#define HAND_STEP_SPEED_DEFAULT 1.0f

#define HAND_CORRECTION_AMP_MIN 0.0f
#define HAND_CORRECTION_AMP_MAX 1.0f
#define HAND_CORRECTION_AMP_DEFAULT 0.32f

#define HAND_INERTIA_MIN 0.0f
#define HAND_INERTIA_MAX 0.95f
#define HAND_INERTIA_DEFAULT 0.90f

#define HAND_BLUR_RESPONSE_MIN 0.0f
#define HAND_BLUR_RESPONSE_MAX 5.0f
#define HAND_BLUR_RESPONSE_DEFAULT 0.45f

// -------------------------------------------------------------------------------------------------
// #1 - Unified Mode Registration (X-Macros)
// -------------------------------------------------------------------------------------------------
// Canonical field order: X(ID, suffix, display_name, tech_name)
//   - ID:          Uppercased enum suffix  (e.g., VIDICON -> MODE_VIDICON)
//   - suffix:      Lowercase C identifier for function prefix (e.g., vidicon_cache, vidicon_render)
//   - display_name: Human-readable string shown in OBS UI dropdown
//   - tech_name:   Shader technique name string (must match data/emulens.effect)
// To add a new effect: Just add a line here, then implement the strategy files.
#define EMULENS_MODES                                                          \
  X(VIDICON, vidicon, "Vidicon", "DrawVidicon")                                 \
  X(STARBURST, starburst, "Starburst", "DrawStarburst")                         \
  X(LEAK, leak, "Light Leak", "DrawLightLeak")                                  \
  X(HANDHELD, handheld, "Handheld", "DrawHandheld")                             \
  X(BOKEH, bokeh, "Bokeh Blur", "DrawBokeh")                                    \
  X(PARTICLES, particles, "Particles", "DrawBokehParticles")                    \
  X(ORTON, orton, "Orton Glow", "DrawOrton")                                    \
  X(GRIT, grit, "Film Grit", "DrawGrit")                                        \
  X(MIRAGE, mirage, "Mirage", "DrawMirage")                                     \
  X(SPOT, spot, "Center Spot", "DrawCenterSpot")                                \
  X(BLOOM, bloom, "Bloom/Glow", "DrawBloomCombine")                             \
  X(PINHOLE, pinhole, "Pinhole", "DrawPinhole")                                 \
  X(POLAROID, polaroid, "Polaroid", "DrawPolaroid")                             \
  X(DAGUERRE, daguerreotype, "Daguerreotype", "DrawDaguerreotype")              \
  X(TECHNICOLOR, technicolor, "Technicolor", "DrawTechnicolor")              \
  X(WET_LENS, wet_lens, "Wet Lens", "DrawWetLens")                       \
  X(PHONE_WET, phone_wet, "Phone Wet Lens", "DrawPhoneWet")
// -------------------------------------------------------------------------------------------------
// #2 - Shader Parameter Constants
// -------------------------------------------------------------------------------------------------
#define P_ELAPSED_TIME "elapsed_time"
#define P_UV_PIXEL_INTERVAL "uv_pixel_interval"
#define P_NOISE_TEX "noise_tex"
#define P_MAIN_TEX "image"
#define P_VIEW_PROJ "ViewProj"
#define P_BLUR_RADIUS "val_blur_radius"
#define P_BLUR_IMAGE "blurred_image"

// Bloom params
#define P_BLOOM_THRESHOLD "bloom_threshold"
#define P_BLOOM_INTENSITY "bloom_intensity"
#define P_BLOOM_RADIUS "bloom_radius"
#define P_BLOOM_SOFTNESS "bloom_softness"
#define P_BLOOM_DUST_AMOUNT "bloom_dust_amount"
#define P_BLOOM_DUST_SCALE "bloom_dust_scale"
#define P_BLOOM_HAZE_AMOUNT "bloom_haze_amount"
#define P_BLOOM_DUST_CONTRAST "bloom_dust_contrast"
#define P_BLOOM_REFRACT_AMOUNT "bloom_refract_amount"
#define P_BLOOM_REFRACT_SCALE "bloom_refract_scale"
#define P_BLOOM_HIGHLIGHTS_ONLY "bloom_highlights_only"
#define P_BLOOM_ANAMORPHIC "bloom_anamorphic"
#define P_BLOOM_COLOR "bloom_color"
#define P_BLOOM_DUST_SPECULAR "bloom_dust_specular"
#define P_BLOOM_DUST_DEPTH_LAYERS "bloom_dust_depth_layers"
#define P_BLOOM_GHOST_AMOUNT "bloom_ghost_amount"
#define P_BLOOM_GHOST_SCALE "bloom_ghost_scale"
#define P_BLOOM_GHOST_COLOR "bloom_ghost_color"


// Wet Lens: separate optically motivated droplet mode, appended so existing
// OBS persisted mode indices never change.
#define P_WET_DENSITY "wet_density"
#define P_WET_SIZE "wet_size"
#define P_WET_REFRACTION "wet_refraction"
#define P_WET_GLINT "wet_glint"
#define P_WET_STRETCH "wet_stretch"
#define P_WET_CHROMATIC "wet_chromatic"
#define P_WET_SEED "wet_seed"

// Larger non-image-forming water structures and highlight veiling for compact
// camera lenses. A separate mode: old Wet Lens scenes remain unchanged.
#define P_PHONE_COVERAGE "phone_coverage"
#define P_PHONE_SCALE "phone_scale"
#define P_PHONE_REFRACTION "phone_refraction"
#define P_PHONE_RIBBONS "phone_ribbons"
#define P_PHONE_REACH "phone_reach"
#define P_PHONE_GLOW "phone_glow"
#define P_PHONE_ARCS "phone_arcs"
#define P_PHONE_ANGLE "phone_angle"
#define P_PHONE_SEED "phone_seed"


// Vidicon params
#define P_VID_HISTORY "history_image"
#define P_VID_DECAY "vid_decay"
#define P_VID_THRESHOLD "vid_threshold"
#define P_VID_COLOR "vid_color"
#define P_VID_BLEND "vid_blend"
#define P_VID_RED_LAG "vid_red_lag"
#define P_VID_BLUE_LAG "vid_blue_lag"
#define P_VID_COMET "vid_comet"
#define P_VID_COMET_LENGTH "vid_comet_length"
#define P_VID_NOISE "vid_noise"
#define P_VID_BEAM_PULL "vid_beam_pull"
#define P_VID_LAG_NONLINEAR "vid_lag_nonlinear"
#define P_VID_REGISTER "vid_register"
#define P_VID_REGISTER_DRIFT "vid_register_drift"
#define P_VID_SCANLINES "vid_scanlines"
#define P_VID_SOURCE_COLOR "vid_source_color"
#define P_VID_GAMMA "vid_gamma"
#define P_VID_SMEAR_ENABLE "vid_smear_enable"
#define P_VID_SMEAR_MODE "vid_smear_mode"
#define P_VID_SMEAR_DIRECTION "vid_smear_direction"
#define P_VID_SMEAR_STRENGTH "vid_smear_strength"
#define P_VID_HALATION "vid_halation"
#define P_VID_HUM "vid_hum"

// Starburst params
#define P_SB_THRESHOLD "sb_threshold"
#define P_SB_INTENSITY "sb_intensity"
#define P_SB_POINTS "sb_points"
#define P_SB_RAY_LENGTH "sb_ray_length"
#define P_SB_THICKNESS "sb_thickness"

// Shared Texrender Slots
#define BLOOM_OUTPUT_SLOT 0
#define BLOOM_EXTRACT_SLOT 1
#define P_SB_SMOOTHNESS "sb_smoothness"
#define P_SB_COLORIZE "sb_colorize"
#define P_SB_COLOR "sb_color"
#define P_SB_CHROMA "sb_chroma"
#define P_SB_ROT_SPEED "sb_rot_speed"
#define P_SB_PULSE_SPEED "sb_pulse_speed"
#define P_SB_PULSE_AMP "sb_pulse_amp"
#define P_SB_SAMPLES "sb_samples"
#define P_SB_ANAMORPHIC "sb_anamorphic"
#define P_SB_ROTATION "sb_rotation"
#define P_SB_BLEND_MODE "sb_blend_mode"

// Mirage params
#define P_MIR_COUNT "mir_count"
#define P_MIR_RADIUS "mir_radius"
#define P_MIR_ROTATION "mir_rotation"
#define P_MIR_ROT_SPEED "mir_rot_speed"
#define P_MIR_CENTER_MASK "mir_center_mask"
#define P_MIR_PARALLEL "mir_parallel"

// Grit params
#define P_GRIT_DUST "grit_dust"
#define P_GRIT_SCRATCHES "grit_scratches"
#define P_GRIT_NOISE "grit_noise"
#define P_GRIT_FLARES "grit_flares"
#define P_GRIT_COLOR "grit_color"
#define P_GRIT_BRIGHTNESS "grit_brightness"
#define P_GRIT_CONTRAST "grit_contrast"
#define P_GRIT_SATURATION "grit_saturation"
#define P_GRIT_OPACITY "grit_opacity"
#define P_GRIT_INSTABILITY "grit_instability"
#define P_GRIT_BLEND_MODE "grit_blend_mode"
#define P_GRIT_SCRATCH_TYPE "grit_scratch_type"
#define P_GRIT_DUST_SIZE "grit_dust_size"

// Handheld computed-output params (CPU-side, not user settings)

// Particles params
#define P_PART_DENSITY "part_density"
#define P_PART_SIZE "part_size"
#define P_PART_SIZE_VAR "part_size_var"
#define P_PART_SPEED "part_speed"
#define P_PART_WIND_X "part_wind_x"
#define P_PART_WIND_Y "part_wind_y"
#define P_PART_WOBBLE "part_wobble"
#define P_PART_COLOR_START "part_color_start"
#define P_PART_COLOR_END "part_color_end"
#define P_PART_SOURCE_COLOR "part_source_color"
#define P_PART_SOURCE_INFLUENCE "part_source_influence"
#define P_PART_SOURCE_BRIGHTNESS "part_source_brightness"
#define P_PART_SOURCE_THRESHOLD "part_source_threshold"
#define P_PART_SOURCE_STRENGTH "part_source_strength"
#define P_PART_FOCUS_X "part_focus_x"
#define P_PART_FOCUS_Y "part_focus_y"
#define P_PART_FOCUS_STRENGTH "part_focus_strength"
#define P_PART_BLUR_EDGE "part_blur_edge"
#define P_PART_USE_POLY "part_use_poly"
#define P_PART_POLY_SIDES "part_poly_sides"
#define P_PART_POLY_ROT "part_poly_rot"
#define P_PART_POLY_ROT_SPEED "part_poly_rot_speed"
#define P_PART_CA_STRENGTH "part_ca_strength"

// Pinhole params
#define P_PINHOLE_SOFTNESS "pinhole_softness"
#define P_PINHOLE_VIGNETTE "pinhole_vignette"
#define P_PINHOLE_DISTORTION "pinhole_distortion"
#define P_PINHOLE_SEPIA "pinhole_sepia"
#define P_PINHOLE_GRAIN "pinhole_grain"

// Polaroid params
#define P_POLAROID_SOFTNESS "polaroid_softness"
#define P_POLAROID_VIGNETTE "polaroid_vignette"
#define P_POLAROID_COLOR_SHIFT "polaroid_color_shift"
#define P_POLAROID_CONTRAST "polaroid_contrast"
#define P_POLAROID_GRAIN "polaroid_grain"
#define P_POLAROID_DEVELOP "polaroid_develop"
#define P_POLAROID_DEVELOP_CENTER "polaroid_develop_center"
#define P_POLAROID_BASE_COLOR "polaroid_base_color"
#define P_POLAROID_BLEED "polaroid_bleed"

// Daguerreotype params
#define P_DAG_CONTRAST "dag_contrast"
#define P_DAG_GRAIN "dag_grain"
#define P_DAG_TINT "dag_tint"
#define P_DAG_VIGNETTE "dag_vignette"
#define P_DAG_EDGE_BLUR "dag_edge_blur"

// Technicolor params
#define P_TECH_STRENGTH "tech_strength"
#define P_TECH_SATURATION "tech_saturation"
#define P_TECH_OFFSET "tech_offset"
#define P_TECH_GLOW "tech_glow"

// Spot params
#define P_SPOT_RADIUS "spot_radius"
#define P_SPOT_SOFTNESS "spot_softness"
#define P_SPOT_ZOOM_STRENGTH "spot_zoom_strength"
#define P_SPOT_BLUR_AMOUNT "spot_blur_amount"
#define P_SPOT_BLUR_RADIUS "spot_blur_radius"
#define P_SPOT_TINT "spot_tint"
#define P_SPOT_BLEND_MODE "spot_blend_mode"

// Leak params
#define P_LEAK_INTENSITY "leak_intensity"
#define P_LEAK_SPEED "leak_speed"
#define P_LEAK_SCALE "leak_scale"
#define P_LEAK_COLOR "leak_color"
#define P_LEAK_COLOR_ENABLE_SHIFT "leak_color_enable_shift"
#define P_LEAK_COLOR_SHIFT "leak_color_shift"
#define P_LEAK_COLOR_SPEED "leak_color_speed"
#define P_LEAK_BIAS_TOP "leak_bias_top"
#define P_LEAK_BIAS_BOT "leak_bias_bot"
#define P_LEAK_BIAS_LEFT "leak_bias_left"
#define P_LEAK_BIAS_RIGHT "leak_bias_right"
#define P_LEAK_EDGE_FALLOFF "leak_edge_falloff"
#define P_LEAK_STREAKINESS "leak_streakiness"
#define P_LEAK_CONTRAST "leak_contrast"
#define P_LEAK_COMPLEXITY "leak_complexity"
#define P_LEAK_BLEND_MODE "leak_blend_mode"

// Bokeh params
#define P_BOKEH_RADIUS "bokeh_radius"
#define P_BOKEH_POWER "bokeh_power"
#define P_BOKEH_SHAPE "bokeh_shape"
#define P_BOKEH_FOCUS_X "bokeh_focus_x"
#define P_BOKEH_FOCUS_Y "bokeh_focus_y"
#define P_BOKEH_WIDTH "bokeh_width"
#define P_BOKEH_FALLOFF "bokeh_falloff"
#define P_BOKEH_BOOST "bokeh_boost"
#define P_BOKEH_HIGH_RESPONSE "bokeh_high_response"
#define P_BOKEH_THRESHOLD "bokeh_threshold"

// Handheld params
#define P_HAND_POS_AMP "hand_pos_amp"
#define P_HAND_POS_SPEED "hand_pos_speed"
#define P_HAND_ROT_AMP "hand_rot_amp"
#define P_HAND_ROT_SPEED "hand_rot_speed"
#define P_HAND_ZOOM_AMP "hand_zoom_amp"
#define P_HAND_ZOOM_SPEED "hand_zoom_speed"
#define P_HAND_BLUR_ENABLE "hand_blur_enable"
#define P_HAND_BLUR_AMP "hand_blur_amp"
#define P_HAND_BLUR_SPEED "hand_blur_speed"
#define P_HAND_BLUR_STATIC "hand_blur_static"
#define P_HAND_FEATHER "hand_feather"
#define P_HAND_TREMOR_AMP "hand_tremor_amp"
#define P_HAND_BREATH_AMP "hand_breath_amp"
#define P_HAND_STEP_AMP "hand_step_amp"
#define P_HAND_STEP_SPEED "hand_step_speed"
#define P_HAND_CORRECTION_AMP "hand_correction_amp"
#define P_HAND_INERTIA "hand_inertia"
#define P_HAND_BLUR_RESPONSE "hand_blur_response"
#define P_HAND_VEL_DIR_X "hand_vel_dir_x"
#define P_HAND_VEL_DIR_Y "hand_vel_dir_y"
#define P_HAND_VEL_MAG "hand_vel_mag"
#define P_HAND_DIR_BLUR_ENABLE "hand_dir_blur_enable"
#define P_HAND_BLUR_DIRECTIONAL "hand_blur_directional"
#define P_HAND_POS_OFFSET "hand_pos_offset"
#define P_HAND_ROT_RAD "hand_rot_rad"
#define P_HAND_ZOOM_FACTOR "hand_zoom_factor"

// Orton params
#define P_ORTON_BLUR_RADIUS "orton_spread"
#define P_ORTON_BLUR_STRENGTH "orton_opacity"
#define P_ORTON_CONTRAST "orton_contrast"
#define P_ORTON_BRIGHTNESS "orton_brightness"
#define P_ORTON_SATURATION "orton_saturation"
#define P_ORTON_BLEND_MODE "orton_blend_mode"
#define P_ORTON_HIGHLIGHT_BOOST "orton_highlight_boost"

// Global property names
#define UI_MODE "mode"
#define UI_PRESET_LIST "preset_list"
#define UI_VERSION_INFO "version_info"

// OBS stores color values (from obs_properties_add_color_alpha) as a 32-bit
// integer in little-endian byte order: bytes [R, G, B, A] (i.e. the int
// reads as 0xAABBGGRR on little-endian platforms). This matches OBS's own
// vec4_from_rgba helper (libobs/graphics/vec4.h), which uses memcpy to
// decode without depending on host byte order. We do the same here.
// Output: out->x=R, out->y=G, out->z=B, out->w=A (RGBA, [0..1]).
// Shaders must bind this as float4 with w=alpha (standard OBS convention).
// CONTRACT: Only accepts values from obs_properties_add_color_alpha.
// Using obs_properties_add_color (no alpha) will produce w=0 (invisible).
static inline void emulens_color_to_vec4(struct vec4 *out, uint32_t rgba_le) {
    uint8_t bytes[4];
    memcpy(bytes, &rgba_le, sizeof(bytes));
    out->x = (float)bytes[0] / 255.0f;
    out->y = (float)bytes[1] / 255.0f;
    out->z = (float)bytes[2] / 255.0f;
    out->w = (float)bytes[3] / 255.0f;
}

// Null-safe parameter setters with runtime validation
// We don't use token pasting for the warning flag to avoid macro complexity
// errors
#define SET_PARAM_DIAGNOSTIC(param, setter, val, type_name, expected_size)     \
  do {                                                                         \
    if (param) {                                                               \
      struct gs_effect_param_info info;                                        \
      gs_effect_get_param_info(param, &info);                                  \
      if (info.type == GS_SHADER_PARAM_UNKNOWN) {                              \
        blog(LOG_WARNING,                                                       \
             "[EmuLensFX] Skip '%s': param type UNKNOWN (stale handle — "       \
             "possible shader recompile?)",                                     \
             info.name ? info.name : "?");                                     \
        break;                                                                 \
      }                                                                        \
      /* Use default_val.num (set at shader compile, never reset) rather */     \
      /* than cur_val.num (reset to 0 after every gs_technique_end) */         \
      size_t actual_size = gs_effect_get_default_val_size(param);              \
      if ((expected_size == 0) !=                                             \
          (info.type == GS_SHADER_PARAM_TEXTURE)) {                            \
        blog(LOG_ERROR,                                                        \
             "[EmuLensFX] Type Mismatch: Passing %s to parameter '%s' (type=%d)", \
             type_name, info.name ? info.name : "?", (int)info.type);          \
        break;                                                                 \
      } else if (info.type != GS_SHADER_PARAM_TEXTURE && actual_size == 0) {   \
        /* Parameter optimized out or inactive; skip to prevent OBS warnings */ \
        break;                                                                 \
      } else if (expected_size > 0 && actual_size != expected_size) {          \
        blog(LOG_ERROR,                                                        \
             "[EmuLensFX] Type Mismatch: Passing %s to parameter '%s' "        \
             "with size %zu (expected %d)",                                    \
             type_name, info.name ? info.name : "?", actual_size,              \
             expected_size);                                                    \
        break;                                                                 \
      }                                                                        \
      setter(param, val);                                                      \
    } else {                                                                   \
      blog(LOG_DEBUG,                                                          \
           "[EmuLensFX] SET skipped: %s handle is NULL (param absent in shader)", \
           type_name);                                                         \
    }                                                                          \
  } while (0)

#define SET_PARAM_FLOAT(param, val)                                            \
  SET_PARAM_DIAGNOSTIC(param, gs_effect_set_float, val, "FLOAT", 4)
#define SET_PARAM_INT(param, val)                                              \
  SET_PARAM_DIAGNOSTIC(param, gs_effect_set_int, val, "INT", 4)
// bool size varies across backends (4 bytes on D3D11/HLSL, 1 byte on OpenGL).
// Skip the size check entirely and validate the shader param type instead.
// NOTE: All break statements below exit the enclosing do{}while(0).
// This is intentional — do not remove the outer do{}while(0).
#define SET_PARAM_BOOL(param, val)                                             \
  do {                                                                         \
    if (param) {                                                               \
      struct gs_effect_param_info info;                                        \
      gs_effect_get_param_info(param, &info);                                  \
      if (info.type == GS_SHADER_PARAM_UNKNOWN) {                              \
        blog(LOG_WARNING,                                                       \
             "[EmuLensFX] Skip '%s': param type UNKNOWN (stale handle)",       \
             info.name ? info.name : "?");                                     \
        break;                                                                 \
      }                                                                        \
      if (info.type == GS_SHADER_PARAM_INT) {                                  \
        gs_effect_set_int(param, val ? 1 : 0);                                 \
      } else if (info.type == GS_SHADER_PARAM_BOOL) {                          \
        gs_effect_set_bool(param, val);                                        \
      } else {                                                                 \
        blog(LOG_ERROR,                                                         \
             "[EmuLensFX] Type Mismatch: Passing BOOL to parameter '%s' (type=%d)", \
             info.name ? info.name : "?", (int)info.type);                     \
        break;                                                                 \
      }                                                                        \
    } else {                                                                   \
      blog(LOG_DEBUG,                                                          \
           "[EmuLensFX] SET skipped: BOOL handle is NULL (param absent in shader)"); \
    }                                                                          \
  } while (0)
#define SET_PARAM_VEC2(param, val)                                             \
  SET_PARAM_DIAGNOSTIC(param, gs_effect_set_vec2, val, "VEC2", 8)
#define SET_PARAM_VEC4(param, val)                                             \
  SET_PARAM_DIAGNOSTIC(param, gs_effect_set_vec4, val, "VEC4", 16)
#define SET_PARAM_TEXTURE(param, val)                                          \
  SET_PARAM_DIAGNOSTIC(param, gs_effect_set_texture, val, "TEXTURE", 0)
#define SET_PARAM_TEXTURE_WITH_FEEDBACK(param, val)                            \
  SET_PARAM_DIAGNOSTIC(param, gs_effect_set_texture, val, "TEXTURE_FB", 0)
#define SET_PARAM_MATRIX4(param, val)                                          \
  SET_PARAM_DIAGNOSTIC(param, gs_effect_set_matrix4, val, "MATRIX4", 64)

// Convenience: read a double setting, clamp to [lo, hi], upload to a
// float param. Replaces the most common SET_PARAM_FLOAT pattern across
// strategy update_settings (clamp_float + obs_data_get_double). The
// underlying SET_PARAM_DIAGNOSTIC handles NULL-param / type-mismatch.
#define SET_CLAMPED_DOUBLE(param, settings, key, lo, hi)                        \
  SET_PARAM_FLOAT(param, clamp_float((float)obs_data_get_double(settings, key), \
                                     (lo), (hi)))

// Convenience: read an int setting, clamp to [lo, hi], upload to a
// float param. Used by blend_mode / shape / count params that are
// stored as int settings but fed to float uniforms.
#define SET_CLAMPED_INT_AS_FLOAT(param, settings, key, lo, hi)                  \
  SET_PARAM_FLOAT(param,                                                        \
                  clamp_float((float)obs_data_get_int(settings, key), (lo), (hi)))

// Convenience: read a bool setting, upload as 0/1 float. Used by
// bool toggles that drive float uniforms (colorize, anamorphic, etc.).
#define SET_BOOL_AS_FLOAT(param, settings, key)                                \
  SET_PARAM_FLOAT(param, (float)obs_data_get_bool(settings, key))

// ────────────────────────────────────────────────────────────────────────────
//  Preset Helper
//  Usage: ADD_PRESET_ENTRY(list, "DisplayKey", "preset_file", "DescriptionKey")
//  Fetches localisation keys via obs_module_text and checks file existence
//  via obs_module_file.  No-op if the JSON file does not exist on disk.
// ────────────────────────────────────────────────────────────────────────────
#define ADD_PRESET_ENTRY(list, name_key, file_name, desc_key)                   \
  do {                                                                          \
    char *_full_pe = obs_module_file("presets/" file_name ".json");            \
    if (_full_pe) {                                                             \
      bfree(_full_pe);                                                          \
      struct dstr _ds_pe = {0};                                                 \
      dstr_printf(&_ds_pe, "%s - %s", obs_module_text(name_key),               \
                  obs_module_text(desc_key));                                   \
      obs_property_list_add_string(list, _ds_pe.array, file_name);              \
      dstr_free(&_ds_pe);                                                       \
    }                                                                           \
  } while (0)
