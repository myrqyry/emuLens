#include "../obs-utils.h"

void spot_cache(struct lens_data *filter, gs_effect_t *effect) {
  GET_PARAM_SAFE(filter->mode_params.spot.radius, effect, P_SPOT_RADIUS);
  GET_PARAM_SAFE(filter->mode_params.spot.softness, effect, P_SPOT_SOFTNESS);
  GET_PARAM_SAFE(filter->mode_params.spot.zoom_strength, effect,
                 P_SPOT_ZOOM_STRENGTH);
  GET_PARAM_SAFE(filter->mode_params.spot.blur_amount, effect,
                 P_SPOT_BLUR_AMOUNT);
  GET_PARAM_SAFE(filter->mode_params.spot.blur_radius, effect,
                 P_SPOT_BLUR_RADIUS);
  GET_PARAM_SAFE(filter->mode_params.spot.tint, effect, P_SPOT_TINT);
  GET_PARAM_SAFE(filter->mode_params.spot.blend_mode, effect,
                 P_SPOT_BLEND_MODE);
}

void spot_update(struct lens_data *filter, obs_data_t *settings) {
  SET_CLAMPED_DOUBLE(filter->mode_params.spot.radius, settings, P_SPOT_RADIUS,
                     0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.spot.softness, settings,
                     P_SPOT_SOFTNESS, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.spot.zoom_strength, settings,
                     P_SPOT_ZOOM_STRENGTH, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.spot.blur_amount, settings,
                     P_SPOT_BLUR_AMOUNT, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.spot.blur_radius, settings,
                     P_SPOT_BLUR_RADIUS, 0.0f, 20.0f);
  uint32_t c_val = (uint32_t)obs_data_get_int(settings, P_SPOT_TINT);
  struct vec4 _c_vec_tint;
  emulens_color_to_vec4(&_c_vec_tint, c_val);
  SET_PARAM_VEC4(filter->mode_params.spot.tint, &_c_vec_tint);
  SET_CLAMPED_INT_AS_FLOAT(filter->mode_params.spot.blend_mode, settings,
                           P_SPOT_BLEND_MODE, 0.0f, 2.0f);
}

gs_texture_t *spot_render(struct lens_data *filter, gs_effect_t *effect,
                          gs_texture_t *source_tex, uint32_t width,
                          uint32_t height) {
  UNUSED_PARAMETER(effect);
  return render_passthrough_to_texrender(
      filter->texrenders[emulens_atomic_load(&filter->cur_write)], filter->tech.spot, filter,
      source_tex, width, height);
}

void spot_destroy_cpu(struct lens_data *filter) { (void)filter; }
void spot_destroy(struct lens_data *filter) { (void)filter; }

void spot_get_defaults(obs_data_t *settings) {
  obs_data_set_default_double(settings, P_SPOT_RADIUS, 0.15);
  obs_data_set_default_double(settings, P_SPOT_SOFTNESS, 0.14);
  obs_data_set_default_double(settings, P_SPOT_ZOOM_STRENGTH, 0.0);
  obs_data_set_default_double(settings, P_SPOT_BLUR_AMOUNT, 0.58);
  obs_data_set_default_double(settings, P_SPOT_BLUR_RADIUS, 3.8);
  obs_data_set_default_int(settings, P_SPOT_BLEND_MODE, 0);
  obs_data_set_default_int(settings, P_SPOT_TINT, 0xFFFFFFFF);
}

void spot_populate_presets(obs_property_t *list) {
  ADD_PRESET_ENTRY(list, "SpotDefault", "default_spot",
                   "SpotDefaultDesc");
  ADD_PRESET_ENTRY(list, "SpotVignette", "spot_vignette",
                   "SpotVignetteDesc");
  ADD_PRESET_ENTRY(list, "SpotMist", "spot_mist",
                    "SpotMistDesc");
  ADD_PRESET_ENTRY(list, "SpotTunnel", "spot_tunnel",
                    "SpotTunnelDesc");
}
