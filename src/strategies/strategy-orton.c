#include "../lens-data.h"

void orton_cache(struct lens_data *filter, gs_effect_t *effect) {
  GET_PARAM_SAFE(filter->mode_params.orton.opacity, effect,
                 P_ORTON_BLUR_STRENGTH);
  GET_PARAM_SAFE(filter->mode_params.orton.contrast, effect, P_ORTON_CONTRAST);
  GET_PARAM_SAFE(filter->mode_params.orton.brightness, effect,
                 P_ORTON_BRIGHTNESS);
  GET_PARAM_SAFE(filter->mode_params.orton.saturation, effect,
                 P_ORTON_SATURATION);
  GET_PARAM_SAFE(filter->mode_params.orton.blend_mode, effect,
                 P_ORTON_BLEND_MODE);
  GET_PARAM_SAFE(filter->mode_params.orton.highlight_boost, effect,
                 P_ORTON_HIGHLIGHT_BOOST);
}

static void orton_bind(struct lens_data *filter) {
  SET_PARAM_FLOAT(filter->mode_params.orton.opacity,
                  filter->runtime_state.orton.prev_opacity);
  SET_PARAM_FLOAT(filter->mode_params.orton.contrast,
                  filter->runtime_state.orton.prev_contrast);
  SET_PARAM_FLOAT(filter->mode_params.orton.brightness,
                  filter->runtime_state.orton.prev_brightness);
  SET_PARAM_FLOAT(filter->mode_params.orton.saturation,
                  filter->runtime_state.orton.prev_saturation);
  SET_PARAM_FLOAT(filter->mode_params.orton.blend_mode,
                  filter->runtime_state.orton.prev_blend_mode);
  SET_PARAM_FLOAT(filter->mode_params.orton.highlight_boost,
                  filter->runtime_state.orton.prev_highlight_boost);
}

void orton_update(struct lens_data *filter, obs_data_t *settings) {
  filter->runtime_state.orton.spread = clamp_float(
      (float)obs_data_get_double(settings, P_ORTON_BLUR_RADIUS), 0.0f, 50.0f);
  filter->runtime_state.orton.prev_opacity = clamp_float(
      (float)obs_data_get_double(settings, P_ORTON_BLUR_STRENGTH), 0.0f, 1.0f);
  filter->runtime_state.orton.prev_contrast = clamp_float(
      (float)obs_data_get_double(settings, P_ORTON_CONTRAST), 0.1f, 5.0f);
  filter->runtime_state.orton.prev_brightness = clamp_float(
      (float)obs_data_get_double(settings, P_ORTON_BRIGHTNESS), -1.0f, 1.0f);
  filter->runtime_state.orton.prev_saturation = clamp_float(
      (float)obs_data_get_double(settings, P_ORTON_SATURATION), 0.0f, 5.0f);
  filter->runtime_state.orton.prev_blend_mode = clamp_float(
      (float)obs_data_get_int(settings, P_ORTON_BLEND_MODE), 0.0f, 2.0f);
  filter->runtime_state.orton.prev_highlight_boost = clamp_float(
      (float)obs_data_get_double(settings, P_ORTON_HIGHLIGHT_BOOST), 0.0f, 5.0f);
  orton_bind(filter);
}

/**
 * Orton render - soft dreamy glow using pre-blurred input.
 * Relies on apply_gaussian_blur + blurred_image texture.
 */
gs_texture_t *orton_render(struct lens_data *filter, gs_effect_t *effect,
                           gs_texture_t *source_tex, uint32_t width,
                           uint32_t height) {
  UNUSED_PARAMETER(effect);

  gs_texture_t *blurred = apply_gaussian_blur(
      filter, source_tex, width, height, filter->runtime_state.orton.spread);

  gs_texrender_t *output_buf = filter->texrenders[0];
  if (!output_buf) {
    EMULENS_LOG(LOG_WARNING, "orton: output texrender missing");
    return NULL;
  }

  gs_texrender_reset(output_buf);
  if (!gs_texrender_begin(output_buf, width, height)) {
    EMULENS_LOG(LOG_DEBUG, "orton: output texrender begin failed");
    return NULL;
  }

  struct vec4 clear_color = {0};
  gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);

  bind_common_parameters(filter, source_tex, width, height);

  // Re-bind all mode-specific params after bind_common_parameters (which
  // flushes/resets per-mode shader uniforms on some backends).
  orton_bind(filter);

  // Use consistent SET_PARAM_TEXTURE macro (was using raw gs_effect_set_texture before)
  bool pass_ok;
  if (blurred && filter->common.blurred_image && filter->tech.orton) {
    SET_PARAM_TEXTURE(filter->common.blurred_image, blurred);
    pass_ok = lens_run_technique_quad(filter->tech.orton, width, height);
  } else {
    // Fallback to passthrough if blur failed
    pass_ok = lens_run_technique_quad(filter->tech.draw, width, height);
  }

  gs_texrender_end(output_buf);
  return pass_ok ? gs_texrender_get_texture(output_buf) : source_tex;
}

void orton_destroy_cpu(struct lens_data *filter) { (void)filter; }
void orton_destroy(struct lens_data *filter) { (void)filter; }

void orton_get_defaults(obs_data_t *settings) {
  // Preserve highlight detail by default. The previous additive 0.6-strength
  // baseline clipped ordinary footage before the user touched a control.
  obs_data_set_default_double(settings, P_ORTON_BLUR_RADIUS, 8.0);
  obs_data_set_default_double(settings, P_ORTON_BLUR_STRENGTH, 0.22);
  obs_data_set_default_double(settings, P_ORTON_CONTRAST, 1.03);
  obs_data_set_default_double(settings, P_ORTON_BRIGHTNESS, -0.03);
  obs_data_set_default_double(settings, P_ORTON_SATURATION, 1.02);
  obs_data_set_default_double(settings, P_ORTON_HIGHLIGHT_BOOST, 0.22);
  obs_data_set_default_int(settings, P_ORTON_BLEND_MODE, 1);
}

void orton_populate_presets(obs_property_t *list) {
  ADD_PRESET_ENTRY(list, "OrtonDefault", "default_orton",
                    "OrtonDefaultDesc");
  ADD_PRESET_ENTRY(list, "OrtonAngelic", "orton_angelic",
                    "OrtonAngelicDesc");
  ADD_PRESET_ENTRY(list, "OrtonHeavy", "orton_heavy",
                    "OrtonHeavyDesc");
  ADD_PRESET_ENTRY(list, "OrtonVelvet", "orton_velvet",
                    "OrtonVelvetDesc");
}
