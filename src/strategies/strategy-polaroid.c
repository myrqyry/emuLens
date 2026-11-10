#include "../obs-utils.h"

void polaroid_cache(struct lens_data *filter, gs_effect_t *effect) {
  GET_PARAM_SAFE(filter->mode_params.polaroid.softness, effect, P_POLAROID_SOFTNESS);
  GET_PARAM_SAFE(filter->mode_params.polaroid.vignette, effect, P_POLAROID_VIGNETTE);
  GET_PARAM_SAFE(filter->mode_params.polaroid.color_shift, effect, P_POLAROID_COLOR_SHIFT);
  GET_PARAM_SAFE(filter->mode_params.polaroid.contrast, effect, P_POLAROID_CONTRAST);
  GET_PARAM_SAFE(filter->mode_params.polaroid.grain, effect, P_POLAROID_GRAIN);
  GET_PARAM_SAFE(filter->mode_params.polaroid.develop, effect, P_POLAROID_DEVELOP);
  GET_PARAM_SAFE(filter->mode_params.polaroid.develop_center, effect, P_POLAROID_DEVELOP_CENTER);
  GET_PARAM_SAFE(filter->mode_params.polaroid.base_color, effect, P_POLAROID_BASE_COLOR);
  GET_PARAM_SAFE(filter->mode_params.polaroid.bleed, effect, P_POLAROID_BLEED);
}

void polaroid_update(struct lens_data *filter, obs_data_t *settings) {
  SET_CLAMPED_DOUBLE(filter->mode_params.polaroid.softness, settings,
                     P_POLAROID_SOFTNESS, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.polaroid.vignette, settings,
                     P_POLAROID_VIGNETTE, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.polaroid.color_shift, settings,
                     P_POLAROID_COLOR_SHIFT, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.polaroid.contrast, settings,
                     P_POLAROID_CONTRAST, 0.0f, 2.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.polaroid.grain, settings,
                     P_POLAROID_GRAIN, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.polaroid.develop, settings,
                     P_POLAROID_DEVELOP, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.polaroid.develop_center, settings,
                     P_POLAROID_DEVELOP_CENTER, 0.0f, 1.0f);

  struct vec4 base_color;
  emulens_color_to_vec4(&base_color,
                        (uint32_t)obs_data_get_int(settings,
                                                   P_POLAROID_BASE_COLOR));
  SET_PARAM_VEC4(filter->mode_params.polaroid.base_color, &base_color);

  SET_CLAMPED_DOUBLE(filter->mode_params.polaroid.bleed, settings,
                     P_POLAROID_BLEED, 0.0f, 1.0f);
}

gs_texture_t *polaroid_render(struct lens_data *filter, gs_effect_t *effect,
                              gs_texture_t *source_tex, uint32_t width,
                              uint32_t height) {
  UNUSED_PARAMETER(effect);
  return render_passthrough_to_texrender(
      filter->texrenders[emulens_atomic_load(&filter->cur_write)], filter->tech.polaroid, filter,
      source_tex, width, height);
}
void polaroid_destroy_cpu(struct lens_data *filter) { (void)filter; }
void polaroid_destroy(struct lens_data *filter) { (void)filter; }
void polaroid_get_defaults(obs_data_t *settings) {
  // Instant film should alter tone and dye response before it advertises
  // itself with heavy vignette, blur, or grain.
  obs_data_set_default_double(settings, P_POLAROID_SOFTNESS, 0.18);
  obs_data_set_default_double(settings, P_POLAROID_VIGNETTE, 0.12);
  obs_data_set_default_double(settings, P_POLAROID_COLOR_SHIFT, 0.55);
  obs_data_set_default_double(settings, P_POLAROID_CONTRAST, 1.06);
  obs_data_set_default_double(settings, P_POLAROID_GRAIN, 0.12);
  obs_data_set_default_double(settings, P_POLAROID_DEVELOP, 1.0);
  obs_data_set_default_double(settings, P_POLAROID_DEVELOP_CENTER, 0.5);
  obs_data_set_default_int(settings, P_POLAROID_BASE_COLOR, 0xFFCCDBE0);
  obs_data_set_default_double(settings, P_POLAROID_BLEED, 0.03);
}

void polaroid_populate_presets(obs_property_t *list) {
  ADD_PRESET_ENTRY(list, "PolaroidDefault", "default_polaroid",
                    "PolaroidDefaultDesc");
  ADD_PRESET_ENTRY(list, "PolaroidVintage", "polaroid_vintage",
                    "PolaroidVintageDesc");
  ADD_PRESET_ENTRY(list, "PolaroidExpired", "polaroid_expired",
                    "PolaroidExpiredDesc");
}
