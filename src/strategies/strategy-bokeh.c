#include "../obs-utils.h"

void bokeh_cache(struct lens_data *filter, gs_effect_t *effect) {
  GET_PARAM_SAFE(filter->mode_params.bokeh.radius, effect, P_BOKEH_RADIUS);
  GET_PARAM_SAFE(filter->mode_params.bokeh.power, effect, P_BOKEH_POWER);
  GET_PARAM_SAFE(filter->mode_params.bokeh.shape, effect, P_BOKEH_SHAPE);
  GET_PARAM_SAFE(filter->mode_params.bokeh.focus_x, effect, P_BOKEH_FOCUS_X);
  GET_PARAM_SAFE(filter->mode_params.bokeh.focus_y, effect, P_BOKEH_FOCUS_Y);
  GET_PARAM_SAFE(filter->mode_params.bokeh.width, effect, P_BOKEH_WIDTH);
  GET_PARAM_SAFE(filter->mode_params.bokeh.falloff, effect, P_BOKEH_FALLOFF);
  GET_PARAM_SAFE(filter->mode_params.bokeh.boost, effect, P_BOKEH_BOOST);
  GET_PARAM_SAFE(filter->mode_params.bokeh.high_response, effect,
                 P_BOKEH_HIGH_RESPONSE);
  GET_PARAM_SAFE(filter->mode_params.bokeh.threshold, effect, P_BOKEH_THRESHOLD);
}

void bokeh_update(struct lens_data *filter, obs_data_t *settings) {
  SET_CLAMPED_DOUBLE(filter->mode_params.bokeh.radius, settings,
                     P_BOKEH_RADIUS, 0.0f, 50.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.bokeh.power, settings, P_BOKEH_POWER,
                     0.1f, 10.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.bokeh.shape, settings, P_BOKEH_SHAPE,
                     0.0f, 8.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.bokeh.focus_x, settings,
                     P_BOKEH_FOCUS_X, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.bokeh.focus_y, settings,
                     P_BOKEH_FOCUS_Y, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.bokeh.width, settings, P_BOKEH_WIDTH,
                     0.01f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.bokeh.falloff, settings,
                     P_BOKEH_FALLOFF, 0.001f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.bokeh.boost, settings, P_BOKEH_BOOST,
                     0.0f, 50.0f);
  SET_BOOL_AS_FLOAT(filter->mode_params.bokeh.high_response, settings,
                    P_BOKEH_HIGH_RESPONSE);
  SET_CLAMPED_DOUBLE(filter->mode_params.bokeh.threshold, settings,
                     P_BOKEH_THRESHOLD, 0.0f, 1.0f);
}

gs_texture_t *bokeh_render(struct lens_data *filter, gs_effect_t *effect,
                           gs_texture_t *source_tex, uint32_t width,
                           uint32_t height) {
  UNUSED_PARAMETER(effect);
  if (!filter || !filter->tech.bokeh)
    return NULL;
  return render_passthrough_to_texrender(
      filter->texrenders[emulens_atomic_load(&filter->cur_write)], filter->tech.bokeh, filter,
      source_tex, width, height);
}

void bokeh_destroy_cpu(struct lens_data *filter) { (void)filter; }
void bokeh_destroy(struct lens_data *filter) { (void)filter; }

void bokeh_get_defaults(obs_data_t *settings) {
  obs_data_set_default_double(settings, P_BOKEH_RADIUS, 7.0);
  obs_data_set_default_double(settings, P_BOKEH_POWER, 1.6);
  obs_data_set_default_int(settings, P_BOKEH_SHAPE, 0);
  obs_data_set_default_double(settings, P_BOKEH_FOCUS_X, 0.5);
  obs_data_set_default_double(settings, P_BOKEH_FOCUS_Y, 0.5);
  obs_data_set_default_double(settings, P_BOKEH_WIDTH, 0.30);
  obs_data_set_default_double(settings, P_BOKEH_FALLOFF, 0.28);
  obs_data_set_default_double(settings, P_BOKEH_BOOST, 3.5);
  obs_data_set_default_bool(settings, P_BOKEH_HIGH_RESPONSE, false);
  obs_data_set_default_double(settings, P_BOKEH_THRESHOLD, 0.70);
}

void bokeh_populate_presets(obs_property_t *list) {
  ADD_PRESET_ENTRY(list, "BokehDefault", "default_bokeh",
                    "BokehDefaultDesc");
  ADD_PRESET_ENTRY(list, "BokehTilt", "bokeh_tiltshift",
                    "BokehTiltDesc");
  ADD_PRESET_ENTRY(list, "BokehDream", "bokeh_dreamy",
                    "BokehDreamDesc");
  ADD_PRESET_ENTRY(list, "BokehNeon", "bokeh_neon",
                    "BokehNeonDesc");
}
