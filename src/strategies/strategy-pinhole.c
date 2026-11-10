#include "../obs-utils.h"

void pinhole_cache(struct lens_data *filter, gs_effect_t *effect) {
  GET_PARAM_SAFE(filter->mode_params.pinhole.softness, effect, P_PINHOLE_SOFTNESS);
  GET_PARAM_SAFE(filter->mode_params.pinhole.vignette, effect, P_PINHOLE_VIGNETTE);
  GET_PARAM_SAFE(filter->mode_params.pinhole.distortion, effect, P_PINHOLE_DISTORTION);
  GET_PARAM_SAFE(filter->mode_params.pinhole.sepia, effect, P_PINHOLE_SEPIA);
  GET_PARAM_SAFE(filter->mode_params.pinhole.grain, effect, P_PINHOLE_GRAIN);
}

void pinhole_update(struct lens_data *filter, obs_data_t *settings) {
  SET_CLAMPED_DOUBLE(filter->mode_params.pinhole.softness, settings,
                     P_PINHOLE_SOFTNESS, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.pinhole.vignette, settings,
                     P_PINHOLE_VIGNETTE, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.pinhole.distortion, settings,
                     P_PINHOLE_DISTORTION, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.pinhole.sepia, settings,
                     P_PINHOLE_SEPIA, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.pinhole.grain, settings,
                     P_PINHOLE_GRAIN, 0.0f, 1.0f);
}

gs_texture_t *pinhole_render(struct lens_data *filter, gs_effect_t *effect,
                             gs_texture_t *source_tex, uint32_t width,
                             uint32_t height) {
  UNUSED_PARAMETER(effect);
  return render_passthrough_to_texrender(
      filter->texrenders[emulens_atomic_load(&filter->cur_write)], filter->tech.pinhole, filter,
      source_tex, width, height);
}
void pinhole_destroy_cpu(struct lens_data *filter) { (void)filter; }
void pinhole_destroy(struct lens_data *filter) { (void)filter; }
void pinhole_get_defaults(obs_data_t *settings) {
  // Pinhole is optical softness and falloff first, "old photo" coloration
  // second. Sepia is intentionally almost absent from the baseline.
  obs_data_set_default_double(settings, P_PINHOLE_SOFTNESS, 0.22);
  obs_data_set_default_double(settings, P_PINHOLE_VIGNETTE, 0.14);
  obs_data_set_default_double(settings, P_PINHOLE_DISTORTION, 0.0);
  obs_data_set_default_double(settings, P_PINHOLE_SEPIA, 0.02);
  obs_data_set_default_double(settings, P_PINHOLE_GRAIN, 0.08);
}

void pinhole_populate_presets(obs_property_t *list) {
  ADD_PRESET_ENTRY(list, "PinholeDefault", "default_pinhole",
                    "PinholeDefaultDesc");
  ADD_PRESET_ENTRY(list, "PinholeClassic", "pinhole_classic",
                    "PinholeClassicDesc");
  ADD_PRESET_ENTRY(list, "PinholeAntique", "pinhole_antique",
                    "PinholeAntiqueDesc");
}
