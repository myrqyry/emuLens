#include "../obs-utils.h"

void daguerreotype_cache(struct lens_data *filter, gs_effect_t *effect) {
  GET_PARAM_SAFE(filter->mode_params.daguerreotype.contrast, effect, P_DAG_CONTRAST);
  GET_PARAM_SAFE(filter->mode_params.daguerreotype.grain, effect, P_DAG_GRAIN);
  GET_PARAM_SAFE(filter->mode_params.daguerreotype.tint, effect, P_DAG_TINT);
  GET_PARAM_SAFE(filter->mode_params.daguerreotype.vignette, effect, P_DAG_VIGNETTE);
  GET_PARAM_SAFE(filter->mode_params.daguerreotype.edge_blur, effect, P_DAG_EDGE_BLUR);
}

void daguerreotype_update(struct lens_data *filter, obs_data_t *settings) {
  SET_CLAMPED_DOUBLE(filter->mode_params.daguerreotype.contrast, settings,
                     P_DAG_CONTRAST, 0.0f, 2.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.daguerreotype.grain, settings,
                     P_DAG_GRAIN, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.daguerreotype.tint, settings,
                     P_DAG_TINT, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.daguerreotype.vignette, settings,
                     P_DAG_VIGNETTE, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.daguerreotype.edge_blur, settings,
                     P_DAG_EDGE_BLUR, 0.0f, 1.0f);
}

gs_texture_t *daguerreotype_render(struct lens_data *filter, gs_effect_t *effect,
                                   gs_texture_t *source_tex, uint32_t width,
                                   uint32_t height) {
  UNUSED_PARAMETER(effect);
  return render_passthrough_to_texrender(
      filter->texrenders[emulens_atomic_load(&filter->cur_write)], filter->tech.daguerreotype,
      filter, source_tex, width, height);
}
void daguerreotype_destroy_cpu(struct lens_data *filter) { (void)filter; }
void daguerreotype_destroy(struct lens_data *filter) { (void)filter; }
void daguerreotype_get_defaults(obs_data_t *settings) {
  // dag_contrast is multiplied again in the shader (1 + value * 2), so a
  // moderate control value already produces a strong plate-like tonal curve.
  obs_data_set_default_double(settings, P_DAG_CONTRAST, 0.38);
  obs_data_set_default_double(settings, P_DAG_GRAIN, 0.16);
  obs_data_set_default_double(settings, P_DAG_TINT, 0.72);
  obs_data_set_default_double(settings, P_DAG_VIGNETTE, 0.18);
  obs_data_set_default_double(settings, P_DAG_EDGE_BLUR, 0.18);
}

void daguerreotype_populate_presets(obs_property_t *list) {
  ADD_PRESET_ENTRY(list, "DaguerreDefault", "default_daguerre",
                    "DaguerreDefaultDesc");
  ADD_PRESET_ENTRY(list, "DaguerreSilver", "daguerre_silver",
                    "DaguerreSilverDesc");
  ADD_PRESET_ENTRY(list, "DaguerreTarnished", "daguerre_tarnished",
                    "DaguerreTarnishedDesc");
}
