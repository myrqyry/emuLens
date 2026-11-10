#include "../obs-utils.h"

void phone_wet_cache(struct lens_data *filter, gs_effect_t *effect) {
  GET_PARAM_SAFE(filter->mode_params.phone_wet.coverage, effect, P_PHONE_COVERAGE);
  GET_PARAM_SAFE(filter->mode_params.phone_wet.scale, effect, P_PHONE_SCALE);
  GET_PARAM_SAFE(filter->mode_params.phone_wet.refraction, effect, P_PHONE_REFRACTION);
  GET_PARAM_SAFE(filter->mode_params.phone_wet.ribbons, effect, P_PHONE_RIBBONS);
  GET_PARAM_SAFE(filter->mode_params.phone_wet.reach, effect, P_PHONE_REACH);
  GET_PARAM_SAFE(filter->mode_params.phone_wet.glow, effect, P_PHONE_GLOW);
  GET_PARAM_SAFE(filter->mode_params.phone_wet.arcs, effect, P_PHONE_ARCS);
  GET_PARAM_SAFE(filter->mode_params.phone_wet.angle, effect, P_PHONE_ANGLE);
  GET_PARAM_SAFE(filter->mode_params.phone_wet.seed, effect, P_PHONE_SEED);
}

void phone_wet_update(struct lens_data *filter, obs_data_t *settings) {
  SET_CLAMPED_DOUBLE(filter->mode_params.phone_wet.coverage, settings,
                     P_PHONE_COVERAGE, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.phone_wet.scale, settings,
                     P_PHONE_SCALE, 0.5f, 1.8f);
  SET_CLAMPED_DOUBLE(filter->mode_params.phone_wet.refraction, settings,
                     P_PHONE_REFRACTION, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.phone_wet.ribbons, settings,
                     P_PHONE_RIBBONS, 0.0f, 2.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.phone_wet.reach, settings,
                     P_PHONE_REACH, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.phone_wet.glow, settings,
                     P_PHONE_GLOW, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.phone_wet.arcs, settings,
                     P_PHONE_ARCS, 0.0f, 1.5f);
  SET_CLAMPED_DOUBLE(filter->mode_params.phone_wet.angle, settings,
                     P_PHONE_ANGLE, -80.0f, 80.0f);
  SET_CLAMPED_INT_AS_FLOAT(filter->mode_params.phone_wet.seed, settings,
                           P_PHONE_SEED, 0, 100);
}

gs_texture_t *phone_wet_render(struct lens_data *filter, gs_effect_t *effect,
                               gs_texture_t *source_tex,
                               uint32_t width, uint32_t height) {
  UNUSED_PARAMETER(effect);
  enum gs_cull_mode previous_cull = gs_get_cull_mode();
  gs_set_cull_mode(GS_NEITHER);
  gs_texture_t *output = render_passthrough_to_texrender(
      filter->texrenders[emulens_atomic_load(&filter->cur_write)],
      filter->tech.phone_wet, filter, source_tex, width, height);
  gs_set_cull_mode(previous_cull);
  return output;
}

void phone_wet_destroy_cpu(struct lens_data *filter) { (void)filter; }
void phone_wet_destroy(struct lens_data *filter) { (void)filter; }

void phone_wet_get_defaults(obs_data_t *settings) {
  obs_data_set_default_double(settings, P_PHONE_COVERAGE, 0.48);
  obs_data_set_default_double(settings, P_PHONE_SCALE, 1.0);
  obs_data_set_default_double(settings, P_PHONE_REFRACTION, 0.55);
  obs_data_set_default_double(settings, P_PHONE_RIBBONS, 0.45);
  obs_data_set_default_double(settings, P_PHONE_REACH, 0.58);
  obs_data_set_default_double(settings, P_PHONE_GLOW, 0.24);
  obs_data_set_default_double(settings, P_PHONE_ARCS, 0.38);
  obs_data_set_default_double(settings, P_PHONE_ANGLE, 0.0);
  obs_data_set_default_int(settings, P_PHONE_SEED, 19);
}

void phone_wet_populate_presets(obs_property_t *list) {
  ADD_PRESET_ENTRY(list, "PhoneWetDefault", "default_phone_wet", "PhoneWetDefaultDesc");
  ADD_PRESET_ENTRY(list, "PhoneWetStreetlight", "phone_wet_streetlights", "PhoneWetStreetlightDesc");
  ADD_PRESET_ENTRY(list, "PhoneWetDownpour", "phone_wet_heavy", "PhoneWetDownpourDesc");
}
