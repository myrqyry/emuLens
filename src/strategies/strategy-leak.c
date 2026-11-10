#include "../obs-utils.h"

void leak_cache(struct lens_data *filter, gs_effect_t *effect) {
  GET_PARAM_SAFE(filter->mode_params.leak.intensity, effect, P_LEAK_INTENSITY);
  GET_PARAM_SAFE(filter->mode_params.leak.speed, effect, P_LEAK_SPEED);
  GET_PARAM_SAFE(filter->mode_params.leak.scale, effect, P_LEAK_SCALE);
  GET_PARAM_SAFE(filter->mode_params.leak.color, effect, P_LEAK_COLOR);
  GET_PARAM_SAFE(filter->mode_params.leak.color_enable_shift, effect,
                 P_LEAK_COLOR_ENABLE_SHIFT);
  GET_PARAM_SAFE(filter->mode_params.leak.color_shift, effect,
                 P_LEAK_COLOR_SHIFT);
  GET_PARAM_SAFE(filter->mode_params.leak.color_speed, effect,
                 P_LEAK_COLOR_SPEED);
  GET_PARAM_SAFE(filter->mode_params.leak.bias_top, effect, P_LEAK_BIAS_TOP);
  GET_PARAM_SAFE(filter->mode_params.leak.bias_bot, effect, P_LEAK_BIAS_BOT);
  GET_PARAM_SAFE(filter->mode_params.leak.bias_left, effect, P_LEAK_BIAS_LEFT);
  GET_PARAM_SAFE(filter->mode_params.leak.bias_right, effect, P_LEAK_BIAS_RIGHT);
  GET_PARAM_SAFE(filter->mode_params.leak.edge_falloff, effect,
                 P_LEAK_EDGE_FALLOFF);
  GET_PARAM_SAFE(filter->mode_params.leak.streakiness, effect,
                 P_LEAK_STREAKINESS);
  GET_PARAM_SAFE(filter->mode_params.leak.contrast, effect, P_LEAK_CONTRAST);
  GET_PARAM_SAFE(filter->mode_params.leak.complexity, effect,
                 P_LEAK_COMPLEXITY);
  GET_PARAM_SAFE(filter->mode_params.leak.blend_mode, effect, P_LEAK_BLEND_MODE);
}

void leak_update(struct lens_data *filter, obs_data_t *settings) {
  SET_CLAMPED_DOUBLE(filter->mode_params.leak.intensity, settings,
                     P_LEAK_INTENSITY, 0.0f, 5.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.leak.speed, settings, P_LEAK_SPEED,
                      0.0f, 2.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.leak.scale, settings, P_LEAK_SCALE,
                     0.1f, 20.0f);
  uint32_t c_val = (uint32_t)obs_data_get_int(settings, P_LEAK_COLOR);
  struct vec4 _c_vec_color;
  emulens_color_to_vec4(&_c_vec_color, c_val);
  SET_PARAM_VEC4(filter->mode_params.leak.color, &_c_vec_color);

  SET_BOOL_AS_FLOAT(filter->mode_params.leak.color_enable_shift, settings,
                    P_LEAK_COLOR_ENABLE_SHIFT);
  c_val = (uint32_t)obs_data_get_int(settings, P_LEAK_COLOR_SHIFT);
  struct vec4 _c_vec_color_shift;
  emulens_color_to_vec4(&_c_vec_color_shift, c_val);
  SET_PARAM_VEC4(filter->mode_params.leak.color_shift, &_c_vec_color_shift);
  SET_CLAMPED_DOUBLE(filter->mode_params.leak.color_speed, settings,
                      P_LEAK_COLOR_SPEED, 0.0f, 2.0f);

  SET_CLAMPED_DOUBLE(filter->mode_params.leak.bias_top, settings,
                     P_LEAK_BIAS_TOP, 0.0f, 5.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.leak.bias_bot, settings,
                     P_LEAK_BIAS_BOT, 0.0f, 5.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.leak.bias_left, settings,
                     P_LEAK_BIAS_LEFT, 0.0f, 5.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.leak.bias_right, settings,
                     P_LEAK_BIAS_RIGHT, 0.0f, 5.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.leak.edge_falloff, settings,
                     P_LEAK_EDGE_FALLOFF, 0.1f, 20.0f);

  SET_CLAMPED_DOUBLE(filter->mode_params.leak.streakiness, settings,
                     P_LEAK_STREAKINESS, 0.1f, 20.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.leak.contrast, settings,
                     P_LEAK_CONTRAST, 0.1f, 10.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.leak.complexity, settings,
                     P_LEAK_COMPLEXITY, 1.0f, 10.0f);
  SET_CLAMPED_INT_AS_FLOAT(filter->mode_params.leak.blend_mode, settings,
                           P_LEAK_BLEND_MODE, 0.0f, 4.0f);
}

gs_texture_t *leak_render(struct lens_data *filter, gs_effect_t *effect,
                          gs_texture_t *source_tex, uint32_t width,
                          uint32_t height) {
  UNUSED_PARAMETER(effect);
  return render_passthrough_to_texrender(
      filter->texrenders[emulens_atomic_load(&filter->cur_write)], filter->tech.leak, filter,
      source_tex, width, height);
}

void leak_destroy_cpu(struct lens_data *filter) { (void)filter; }
void leak_destroy(struct lens_data *filter) { (void)filter; }

void leak_get_defaults(obs_data_t *settings) {
  // One localized warm leak reads like stray light; equal pressure from all
  // four edges reads like a procedural overlay.
  obs_data_set_default_double(settings, P_LEAK_INTENSITY, 0.55);
  obs_data_set_default_double(settings, P_LEAK_SPEED, 0.04);
  obs_data_set_default_double(settings, P_LEAK_COLOR_SPEED, 0.02);
  obs_data_set_default_double(settings, P_LEAK_SCALE, 1.4);
  obs_data_set_default_int(settings, P_LEAK_COLOR, 0xFF4488FF);
  obs_data_set_default_bool(settings, P_LEAK_COLOR_ENABLE_SHIFT, false);
  obs_data_set_default_int(settings, P_LEAK_COLOR_SHIFT, 0xFFFFFFFF);
  obs_data_set_default_double(settings, P_LEAK_BIAS_TOP, 0.15);
  obs_data_set_default_double(settings, P_LEAK_BIAS_BOT, 0.10);
  obs_data_set_default_double(settings, P_LEAK_BIAS_LEFT, 0.75);
  obs_data_set_default_double(settings, P_LEAK_BIAS_RIGHT, 0.05);
  obs_data_set_default_double(settings, P_LEAK_EDGE_FALLOFF, 4.5);
  obs_data_set_default_double(settings, P_LEAK_STREAKINESS, 0.75);
  obs_data_set_default_double(settings, P_LEAK_CONTRAST, 1.35);
  obs_data_set_default_int(settings, P_LEAK_COMPLEXITY, 3);
  obs_data_set_default_int(settings, P_LEAK_BLEND_MODE, 2);
}

void leak_populate_presets(obs_property_t *list) {
  ADD_PRESET_ENTRY(list, "LeakDefault", "default_leak",
                    "LeakDefaultDesc");
  ADD_PRESET_ENTRY(list, "LeakWarm", "leak_warm",
                    "LeakWarmDesc");
  ADD_PRESET_ENTRY(list, "LeakChaos", "leak_chaos",
                    "LeakChaosDesc");
  ADD_PRESET_ENTRY(list, "LeakAmber", "leak_amber",
                    "LeakAmberDesc");
}
