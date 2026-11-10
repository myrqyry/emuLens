#include "../obs-utils.h"

void mirage_cache(struct lens_data *filter, gs_effect_t *effect) {
  GET_PARAM_SAFE(filter->mode_params.mirage.count, effect, P_MIR_COUNT);
  GET_PARAM_SAFE(filter->mode_params.mirage.radius, effect, P_MIR_RADIUS);
  GET_PARAM_SAFE(filter->mode_params.mirage.rotation, effect, P_MIR_ROTATION);
  GET_PARAM_SAFE(filter->mode_params.mirage.rot_speed, effect, P_MIR_ROT_SPEED);
  GET_PARAM_SAFE(filter->mode_params.mirage.center_mask, effect,
                 P_MIR_CENTER_MASK);
  GET_PARAM_SAFE(filter->mode_params.mirage.parallel, effect, P_MIR_PARALLEL);
}

void mirage_update(struct lens_data *filter, obs_data_t *settings) {
  SET_CLAMPED_DOUBLE(filter->mode_params.mirage.count, settings, P_MIR_COUNT,
                     3.0f, 12.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.mirage.radius, settings, P_MIR_RADIUS,
                     0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.mirage.rotation, settings,
                     P_MIR_ROTATION, 0.0f, 360.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.mirage.rot_speed, settings,
                     P_MIR_ROT_SPEED, -10.0f, 10.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.mirage.center_mask, settings,
                     P_MIR_CENTER_MASK, 0.0f, 1.0f);
  SET_BOOL_AS_FLOAT(filter->mode_params.mirage.parallel, settings,
                    P_MIR_PARALLEL);
}

gs_texture_t *mirage_render(struct lens_data *filter, gs_effect_t *effect,
                            gs_texture_t *source_tex, uint32_t width,
                            uint32_t height) {
  UNUSED_PARAMETER(effect);
  return render_passthrough_to_texrender(
      filter->texrenders[emulens_atomic_load(&filter->cur_write)], filter->tech.mirage, filter,
      source_tex, width, height);
}

void mirage_destroy_cpu(struct lens_data *filter) { (void)filter; }
void mirage_destroy(struct lens_data *filter) { (void)filter; }

void mirage_get_defaults(obs_data_t *settings) {
  obs_data_set_default_int(settings, P_MIR_COUNT, 4);
  obs_data_set_default_double(settings, P_MIR_RADIUS, 0.035);
  obs_data_set_default_double(settings, P_MIR_ROTATION, 0.0);
  obs_data_set_default_double(settings, P_MIR_ROT_SPEED, 0.0);
  obs_data_set_default_double(settings, P_MIR_CENTER_MASK, 0.28);
  obs_data_set_default_bool(settings, P_MIR_PARALLEL, false);
}

void mirage_populate_presets(obs_property_t *list) {
  ADD_PRESET_ENTRY(list, "MirageDefault", "default_mirage",
                    "MirageDefaultDesc");
  ADD_PRESET_ENTRY(list, "MirageKaleido", "mirage_kaleido",
                    "MirageKaleidoDesc");
  ADD_PRESET_ENTRY(list, "MirageDrunk", "mirage_drunk",
                    "MirageDrunkDesc");
  ADD_PRESET_ENTRY(list, "MirageSplit", "mirage_split",
                    "MirageSplitDesc");
}
