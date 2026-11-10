#include "../obs-utils.h"

void starburst_cache(struct lens_data *filter, gs_effect_t *effect) {
  GET_PARAM_SAFE(filter->mode_params.starburst.threshold, effect, P_SB_THRESHOLD);
  GET_PARAM_SAFE(filter->mode_params.starburst.intensity, effect, P_SB_INTENSITY);
  GET_PARAM_SAFE(filter->mode_params.starburst.points, effect, P_SB_POINTS);
  GET_PARAM_SAFE(filter->mode_params.starburst.ray_length, effect,
                 P_SB_RAY_LENGTH);
  GET_PARAM_SAFE(filter->mode_params.starburst.thickness, effect, P_SB_THICKNESS);
  GET_PARAM_SAFE(filter->mode_params.starburst.smoothness, effect,
                 P_SB_SMOOTHNESS);
  GET_PARAM_SAFE(filter->mode_params.starburst.colorize, effect, P_SB_COLORIZE);
  GET_PARAM_SAFE(filter->mode_params.starburst.color, effect, P_SB_COLOR);
  GET_PARAM_SAFE(filter->mode_params.starburst.chroma, effect, P_SB_CHROMA);
  GET_PARAM_SAFE(filter->mode_params.starburst.rot_speed, effect, P_SB_ROT_SPEED);
  GET_PARAM_SAFE(filter->mode_params.starburst.rotation, effect, P_SB_ROTATION);
  GET_PARAM_SAFE(filter->mode_params.starburst.pulse_speed, effect,
                 P_SB_PULSE_SPEED);
  GET_PARAM_SAFE(filter->mode_params.starburst.pulse_amp, effect, P_SB_PULSE_AMP);
  GET_PARAM_SAFE(filter->mode_params.starburst.samples, effect, P_SB_SAMPLES);
  GET_PARAM_SAFE(filter->mode_params.starburst.anamorphic, effect,
                 P_SB_ANAMORPHIC);
  GET_PARAM_SAFE(filter->mode_params.starburst.blend_mode, effect,
                 P_SB_BLEND_MODE);
}

void starburst_update(struct lens_data *filter, obs_data_t *settings) {
  SET_CLAMPED_DOUBLE(filter->mode_params.starburst.threshold, settings,
                     P_SB_THRESHOLD, SB_THRESHOLD_MIN, SB_THRESHOLD_MAX);
  SET_CLAMPED_DOUBLE(filter->mode_params.starburst.intensity, settings,
                     P_SB_INTENSITY, SB_INTENSITY_MIN, SB_INTENSITY_MAX);
  SET_CLAMPED_DOUBLE(filter->mode_params.starburst.points, settings,
                     P_SB_POINTS, 2.0f, 16.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.starburst.ray_length, settings,
                     P_SB_RAY_LENGTH, 0.01f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.starburst.thickness, settings,
                     P_SB_THICKNESS, 0.1f, 10.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.starburst.smoothness, settings,
                     P_SB_SMOOTHNESS, 0.1f, 10.0f);
  SET_BOOL_AS_FLOAT(filter->mode_params.starburst.colorize, settings,
                    P_SB_COLORIZE);
  uint32_t c_val = (uint32_t)obs_data_get_int(settings, P_SB_COLOR);
  struct vec4 _c_vec_color;
  emulens_color_to_vec4(&_c_vec_color, c_val);
  SET_PARAM_VEC4(filter->mode_params.starburst.color, &_c_vec_color);
  SET_CLAMPED_DOUBLE(filter->mode_params.starburst.chroma, settings,
                     P_SB_CHROMA, 0.0f, 2.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.starburst.rot_speed, settings,
                      P_SB_ROT_SPEED, -3.0f, 3.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.starburst.rotation, settings,
                     P_SB_ROTATION, 0.0f, 360.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.starburst.pulse_speed, settings,
                      P_SB_PULSE_SPEED, 0.0f, 3.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.starburst.pulse_amp, settings,
                     P_SB_PULSE_AMP, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.starburst.samples, settings,
                     P_SB_SAMPLES, 4.0f, 16.0f);
  SET_BOOL_AS_FLOAT(filter->mode_params.starburst.anamorphic, settings,
                    P_SB_ANAMORPHIC);
  SET_CLAMPED_INT_AS_FLOAT(filter->mode_params.starburst.blend_mode, settings,
                           P_SB_BLEND_MODE, 0.0f, 2.0f);
}

gs_texture_t *starburst_render(struct lens_data *filter, gs_effect_t *effect,
                               gs_texture_t *source_tex, uint32_t width,
                               uint32_t height) {
  UNUSED_PARAMETER(effect);
  return render_passthrough_to_texrender(filter->texrenders[0],
                                         filter->tech.starburst, filter,
                                         source_tex, width, height);
}

void starburst_destroy_cpu(struct lens_data *filter) { (void)filter; }
void starburst_destroy(struct lens_data *filter) { (void)filter; }

void starburst_get_defaults(obs_data_t *settings) {
  // Static diffraction is the baseline. Rotation and pulsing are deliberate
  // stylization choices, not defaults.
  obs_data_set_default_double(settings, P_SB_THRESHOLD, SB_THRESHOLD_DEFAULT);
  obs_data_set_default_double(settings, P_SB_INTENSITY, SB_INTENSITY_DEFAULT);
  obs_data_set_default_int(settings, P_SB_POINTS, 6);
  obs_data_set_default_double(settings, P_SB_RAY_LENGTH, 0.16);
  obs_data_set_default_double(settings, P_SB_THICKNESS, 0.9);
  obs_data_set_default_double(settings, P_SB_SMOOTHNESS, 4.5);
  obs_data_set_default_bool(settings, P_SB_COLORIZE, false);
  obs_data_set_default_int(settings, P_SB_COLOR, 0xFFFFFFFF);
  obs_data_set_default_double(settings, P_SB_CHROMA, 0.02);
  obs_data_set_default_double(settings, P_SB_ROT_SPEED, 0.0);
  obs_data_set_default_double(settings, P_SB_ROTATION, 0.0);
  obs_data_set_default_double(settings, P_SB_PULSE_SPEED, 0.18);
  obs_data_set_default_double(settings, P_SB_PULSE_AMP, 0.0);
  obs_data_set_default_int(settings, P_SB_SAMPLES, 12);
  obs_data_set_default_bool(settings, P_SB_ANAMORPHIC, false);
  obs_data_set_default_int(settings, P_SB_BLEND_MODE, 1);
}

void starburst_populate_presets(obs_property_t *list) {
  ADD_PRESET_ENTRY(list, "StarburstDefault", "default_starburst",
                    "StarburstDefaultDesc");
  ADD_PRESET_ENTRY(list, "StarburstAnamorphic", "starburst_anamorphic",
                    "StarburstAnamorphicDesc");
  ADD_PRESET_ENTRY(list, "StarburstGlint", "starburst_glint",
                    "StarburstGlintDesc");
  ADD_PRESET_ENTRY(list, "StarburstCinema", "starburst_cinema",
                    "StarburstCinemaDesc");
}
