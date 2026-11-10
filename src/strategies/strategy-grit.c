#include "../obs-utils.h"

void grit_cache(struct lens_data *filter, gs_effect_t *effect) {
  GET_PARAM_SAFE(filter->mode_params.grit.dust, effect, P_GRIT_DUST);
  GET_PARAM_SAFE(filter->mode_params.grit.scratches, effect, P_GRIT_SCRATCHES);
  GET_PARAM_SAFE(filter->mode_params.grit.noise, effect, P_GRIT_NOISE);
  GET_PARAM_SAFE(filter->mode_params.grit.flares, effect, P_GRIT_FLARES);
  GET_PARAM_SAFE(filter->mode_params.grit.color, effect, P_GRIT_COLOR);
  GET_PARAM_SAFE(filter->mode_params.grit.brightness, effect, P_GRIT_BRIGHTNESS);
  GET_PARAM_SAFE(filter->mode_params.grit.contrast, effect, P_GRIT_CONTRAST);
  GET_PARAM_SAFE(filter->mode_params.grit.saturation, effect, P_GRIT_SATURATION);
  GET_PARAM_SAFE(filter->mode_params.grit.opacity, effect, P_GRIT_OPACITY);
  GET_PARAM_SAFE(filter->mode_params.grit.instability, effect,
                 P_GRIT_INSTABILITY);
  GET_PARAM_SAFE(filter->mode_params.grit.blend_mode, effect, P_GRIT_BLEND_MODE);
  GET_PARAM_SAFE(filter->mode_params.grit.scratch_type, effect, P_GRIT_SCRATCH_TYPE);
  GET_PARAM_SAFE(filter->mode_params.grit.dust_size, effect, P_GRIT_DUST_SIZE);
}

void grit_update(struct lens_data *filter, obs_data_t *settings) {
  SET_CLAMPED_DOUBLE(filter->mode_params.grit.dust, settings, P_GRIT_DUST,
                     0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.grit.scratches, settings,
                     P_GRIT_SCRATCHES, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.grit.noise, settings, P_GRIT_NOISE,
                     0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.grit.flares, settings, P_GRIT_FLARES,
                     0.0f, 1.0f);
  uint32_t c_val = (uint32_t)obs_data_get_int(settings, P_GRIT_COLOR);
  struct vec4 _c_vec_color;
  emulens_color_to_vec4(&_c_vec_color, c_val);
  SET_PARAM_VEC4(filter->mode_params.grit.color, &_c_vec_color);
  SET_CLAMPED_DOUBLE(filter->mode_params.grit.brightness, settings,
                     P_GRIT_BRIGHTNESS, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.grit.contrast, settings,
                     P_GRIT_CONTRAST, 0.0f, 5.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.grit.saturation, settings,
                     P_GRIT_SATURATION, 0.0f, 5.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.grit.opacity, settings,
                     P_GRIT_OPACITY, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.grit.instability, settings,
                     P_GRIT_INSTABILITY, 0.0f, 2.0f);
  SET_CLAMPED_INT_AS_FLOAT(filter->mode_params.grit.blend_mode, settings,
                           P_GRIT_BLEND_MODE, 0.0f, 5.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.grit.scratch_type, settings,
                     P_GRIT_SCRATCH_TYPE, 0.0f, 2.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.grit.dust_size, settings,
                     P_GRIT_DUST_SIZE, 0.1f, 5.0f);
}

gs_texture_t *grit_render(struct lens_data *filter, gs_effect_t *effect,
                          gs_texture_t *source_tex, uint32_t width,
                          uint32_t height) {
  UNUSED_PARAMETER(effect);
  return render_passthrough_to_texrender(
      filter->texrenders[emulens_atomic_load(&filter->cur_write)], filter->tech.grit, filter,
      source_tex, width, height);
}

void grit_destroy_cpu(struct lens_data *filter) { (void)filter; }
void grit_destroy(struct lens_data *filter) { (void)filter; }

void grit_get_defaults(obs_data_t *settings) {
  obs_data_set_default_double(settings, P_GRIT_DUST, 0.07);
  obs_data_set_default_double(settings, P_GRIT_SCRATCHES, 0.03);
  obs_data_set_default_double(settings, P_GRIT_NOISE, 0.12);
  obs_data_set_default_double(settings, P_GRIT_FLARES, 0.0);
  obs_data_set_default_double(settings, P_GRIT_BRIGHTNESS, 0.0);
  obs_data_set_default_double(settings, P_GRIT_CONTRAST, 1.0);
  obs_data_set_default_double(settings, P_GRIT_SATURATION, 0.98);
  obs_data_set_default_double(settings, P_GRIT_OPACITY, 0.30);
  obs_data_set_default_double(settings, P_GRIT_INSTABILITY, 0.10);
  obs_data_set_default_int(settings, P_GRIT_BLEND_MODE, 3);
  obs_data_set_default_int(settings, P_GRIT_COLOR, 0xFF888888);
  obs_data_set_default_int(settings, P_GRIT_SCRATCH_TYPE, 1);
  obs_data_set_default_double(settings, P_GRIT_DUST_SIZE, 0.8);
}

void grit_populate_presets(obs_property_t *list) {
  ADD_PRESET_ENTRY(list, "GritDefault", "default_grit",
                    "GritDefaultDesc");
  ADD_PRESET_ENTRY(list, "Grit1920", "grit_1920s",
                    "Grit1920Desc");
  ADD_PRESET_ENTRY(list, "GritGrind", "grit_grindhouse",
                    "GritGrindDesc");
  ADD_PRESET_ENTRY(list, "GritProjector", "grit_projector",
                    "GritProjectorDesc");
}
