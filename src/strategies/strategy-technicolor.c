#include "../obs-utils.h"

void technicolor_cache(struct lens_data *filter, gs_effect_t *effect) {
  GET_PARAM_SAFE(filter->mode_params.technicolor.strength, effect, P_TECH_STRENGTH);
  GET_PARAM_SAFE(filter->mode_params.technicolor.saturation, effect, P_TECH_SATURATION);
  GET_PARAM_SAFE(filter->mode_params.technicolor.offset, effect, P_TECH_OFFSET);
  GET_PARAM_SAFE(filter->mode_params.technicolor.glow, effect, P_TECH_GLOW);
}

void technicolor_update(struct lens_data *filter, obs_data_t *settings) {
  SET_CLAMPED_DOUBLE(filter->mode_params.technicolor.strength, settings,
                     P_TECH_STRENGTH, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.technicolor.saturation, settings,
                     P_TECH_SATURATION, 0.0f, 3.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.technicolor.offset, settings,
                     P_TECH_OFFSET, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.technicolor.glow, settings,
                     P_TECH_GLOW, 0.0f, 1.0f);
}

gs_texture_t *technicolor_render(struct lens_data *filter, gs_effect_t *effect,
                                 gs_texture_t *source_tex, uint32_t width,
                                 uint32_t height) {
  UNUSED_PARAMETER(effect);
  return render_passthrough_to_texrender(
      filter->texrenders[emulens_atomic_load(&filter->cur_write)], filter->tech.technicolor, filter,
      source_tex, width, height);
}
void technicolor_destroy_cpu(struct lens_data *filter) { (void)filter; }
void technicolor_destroy(struct lens_data *filter) { (void)filter; }
void technicolor_get_defaults(obs_data_t *settings) {
  // Keep the two-strip palette obvious without pushing every channel into
  // clipping; physical misregistration belongs to the named preset.
  obs_data_set_default_double(settings, P_TECH_STRENGTH, 0.72);
  obs_data_set_default_double(settings, P_TECH_SATURATION, 1.06);
  obs_data_set_default_double(settings, P_TECH_OFFSET, 0.02);
  obs_data_set_default_double(settings, P_TECH_GLOW, 0.08);
}

void technicolor_populate_presets(obs_property_t *list) {
  ADD_PRESET_ENTRY(list, "TechDefault", "default_technicolor",
                    "TechDefaultDesc");
  ADD_PRESET_ENTRY(list, "TechClassic", "technicolor_classic",
                    "TechClassicDesc");
  ADD_PRESET_ENTRY(list, "TechMisaligned", "technicolor_misaligned",
                    "TechMisalignedDesc");
}
