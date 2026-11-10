#include "../obs-utils.h"

void wet_lens_cache(struct lens_data *filter, gs_effect_t *effect) {
  GET_PARAM_SAFE(filter->mode_params.wet_lens.density, effect, P_WET_DENSITY);
  GET_PARAM_SAFE(filter->mode_params.wet_lens.size, effect, P_WET_SIZE);
  GET_PARAM_SAFE(filter->mode_params.wet_lens.refraction, effect, P_WET_REFRACTION);
  GET_PARAM_SAFE(filter->mode_params.wet_lens.glint, effect, P_WET_GLINT);
  GET_PARAM_SAFE(filter->mode_params.wet_lens.stretch, effect, P_WET_STRETCH);
  GET_PARAM_SAFE(filter->mode_params.wet_lens.chromatic, effect, P_WET_CHROMATIC);
  GET_PARAM_SAFE(filter->mode_params.wet_lens.seed, effect, P_WET_SEED);
}

void wet_lens_update(struct lens_data *filter, obs_data_t *settings) {
  SET_CLAMPED_DOUBLE(filter->mode_params.wet_lens.density, settings,
                     P_WET_DENSITY, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.wet_lens.size, settings,
                     P_WET_SIZE, 0.25f, 1.25f);
  SET_CLAMPED_DOUBLE(filter->mode_params.wet_lens.refraction, settings,
                     P_WET_REFRACTION, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.wet_lens.glint, settings,
                     P_WET_GLINT, 0.0f, 2.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.wet_lens.stretch, settings,
                     P_WET_STRETCH, 0.0f, 0.8f);
  SET_CLAMPED_DOUBLE(filter->mode_params.wet_lens.chromatic, settings,
                     P_WET_CHROMATIC, 0.0f, 1.0f);
  SET_CLAMPED_INT_AS_FLOAT(filter->mode_params.wet_lens.seed, settings,
                           P_WET_SEED, 0, 100);
}

gs_texture_t *wet_lens_render(struct lens_data *filter, gs_effect_t *effect,
                              gs_texture_t *source_tex,
                              uint32_t width, uint32_t height) {
  UNUSED_PARAMETER(effect);
  // Internal optical quad must remain visible regardless of the culling
  // state inherited from the scene; restore the prior state afterwards.
  enum gs_cull_mode previous_cull = gs_get_cull_mode();
  gs_set_cull_mode(GS_NEITHER);
  gs_texture_t *output = render_passthrough_to_texrender(
      filter->texrenders[emulens_atomic_load(&filter->cur_write)],
      filter->tech.wet_lens, filter, source_tex, width, height);
  gs_set_cull_mode(previous_cull);
  return output;
}

void wet_lens_destroy_cpu(struct lens_data *filter) { (void)filter; }
void wet_lens_destroy(struct lens_data *filter) { (void)filter; }

void wet_lens_get_defaults(obs_data_t *settings) {
  obs_data_set_default_double(settings, P_WET_DENSITY, 0.58);
  obs_data_set_default_double(settings, P_WET_SIZE, 0.86);
  obs_data_set_default_double(settings, P_WET_REFRACTION, 0.65);
  obs_data_set_default_double(settings, P_WET_GLINT, 0.65);
  obs_data_set_default_double(settings, P_WET_STRETCH, 0.24);
  obs_data_set_default_double(settings, P_WET_CHROMATIC, 0.16);
  obs_data_set_default_int(settings, P_WET_SEED, 11);
}

void wet_lens_populate_presets(obs_property_t *list) {
  ADD_PRESET_ENTRY(list, "WetLensDefault", "default_wet_lens",
                   "WetLensDefaultDesc");
  ADD_PRESET_ENTRY(list, "WetLensNightWindow", "wet_lens_night_window",
                   "WetLensNightWindowDesc");
}
