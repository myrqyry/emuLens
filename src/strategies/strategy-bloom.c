#include "../obs-utils.h"

void bloom_cache(struct lens_data *filter, gs_effect_t *effect) {
  GET_PARAM_SAFE(filter->mode_params.bloom.threshold, effect, P_BLOOM_THRESHOLD);
  GET_PARAM_SAFE(filter->mode_params.bloom.intensity, effect, P_BLOOM_INTENSITY);
  GET_PARAM_SAFE(filter->mode_params.bloom.radius, effect, P_BLOOM_RADIUS);
  GET_PARAM_SAFE(filter->mode_params.bloom.softness, effect, P_BLOOM_SOFTNESS);
  GET_PARAM_SAFE(filter->mode_params.bloom.dust_amount, effect,
                 P_BLOOM_DUST_AMOUNT);
  GET_PARAM_SAFE(filter->mode_params.bloom.dust_scale, effect,
                 P_BLOOM_DUST_SCALE);
  GET_PARAM_SAFE(filter->mode_params.bloom.haze_amount, effect,
                 P_BLOOM_HAZE_AMOUNT);
  GET_PARAM_SAFE(filter->mode_params.bloom.dust_contrast, effect,
                 P_BLOOM_DUST_CONTRAST);
  GET_PARAM_SAFE(filter->mode_params.bloom.refract_amount, effect,
                 P_BLOOM_REFRACT_AMOUNT);
  GET_PARAM_SAFE(filter->mode_params.bloom.refract_scale, effect,
                 P_BLOOM_REFRACT_SCALE);
  GET_PARAM_SAFE(filter->mode_params.bloom.highlights_only, effect,
                 P_BLOOM_HIGHLIGHTS_ONLY);
  GET_PARAM_SAFE(filter->mode_params.bloom.anamorphic, effect,
                 P_BLOOM_ANAMORPHIC);
  GET_PARAM_SAFE(filter->mode_params.bloom.color, effect, P_BLOOM_COLOR);
  GET_PARAM_SAFE(filter->mode_params.bloom.dust_specular, effect,
                 P_BLOOM_DUST_SPECULAR);
  GET_PARAM_SAFE(filter->mode_params.bloom.dust_depth_layers, effect,
                 P_BLOOM_DUST_DEPTH_LAYERS);
  GET_PARAM_SAFE(filter->mode_params.bloom.ghost_amount, effect,
                 P_BLOOM_GHOST_AMOUNT);
  GET_PARAM_SAFE(filter->mode_params.bloom.ghost_scale, effect,
                 P_BLOOM_GHOST_SCALE);
  GET_PARAM_SAFE(filter->mode_params.bloom.ghost_color, effect,
                 P_BLOOM_GHOST_COLOR);
}

void bloom_update(struct lens_data *filter, obs_data_t *settings) {
  SET_CLAMPED_DOUBLE(filter->mode_params.bloom.threshold, settings,
                     P_BLOOM_THRESHOLD, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.bloom.intensity, settings,
                     P_BLOOM_INTENSITY, 0.0f, 10.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.bloom.radius, settings,
                     P_BLOOM_RADIUS, 0.0f, 100.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.bloom.softness, settings,
                     P_BLOOM_SOFTNESS, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.bloom.dust_amount, settings,
                     P_BLOOM_DUST_AMOUNT, 0.0f, 2.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.bloom.dust_scale, settings,
                     P_BLOOM_DUST_SCALE, 1.0f, 50.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.bloom.haze_amount, settings,
                     P_BLOOM_HAZE_AMOUNT, 0.0f, 2.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.bloom.dust_contrast, settings,
                     P_BLOOM_DUST_CONTRAST, 0.0f, 2.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.bloom.refract_amount, settings,
                     P_BLOOM_REFRACT_AMOUNT, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.bloom.refract_scale, settings,
                     P_BLOOM_REFRACT_SCALE, 0.1f, 10.0f);
  SET_BOOL_AS_FLOAT(filter->mode_params.bloom.highlights_only, settings,
                    P_BLOOM_HIGHLIGHTS_ONLY);
  SET_BOOL_AS_FLOAT(filter->mode_params.bloom.anamorphic, settings,
                    P_BLOOM_ANAMORPHIC);

  uint32_t c_val = (uint32_t)obs_data_get_int(settings, P_BLOOM_COLOR);
  struct vec4 c_vec;
  emulens_color_to_vec4(&c_vec, c_val);
  SET_PARAM_VEC4(filter->mode_params.bloom.color, &c_vec);

  SET_CLAMPED_DOUBLE(filter->mode_params.bloom.dust_specular, settings,
                     P_BLOOM_DUST_SPECULAR, 0.0f, 2.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.bloom.dust_depth_layers, settings,
                     P_BLOOM_DUST_DEPTH_LAYERS, 1.0f, 5.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.bloom.ghost_amount, settings,
                     P_BLOOM_GHOST_AMOUNT, 0.0f, 2.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.bloom.ghost_scale, settings,
                     P_BLOOM_GHOST_SCALE, 0.1f, 10.0f);
  uint32_t ghost_c_val =
      (uint32_t)obs_data_get_int(settings, P_BLOOM_GHOST_COLOR);
  struct vec4 ghost_c_vec;
  emulens_color_to_vec4(&ghost_c_vec, ghost_c_val);
  SET_PARAM_VEC4(filter->mode_params.bloom.ghost_color, &ghost_c_vec);

  filter->runtime_state.bloom.radius =
      (float)obs_data_get_double(settings, P_BLOOM_RADIUS);
  filter->runtime_state.bloom.anamorphic =
      obs_data_get_bool(settings, P_BLOOM_ANAMORPHIC);
}

void bloom_destroy_cpu(struct lens_data *filter) { (void)filter; }
void bloom_destroy(struct lens_data *filter) { (void)filter; }

static uint32_t bloom_half_dim(uint32_t value) {
  return value > 1 ? (value + 1u) / 2u : 1u;
}

static gs_texture_t *
bloom_render_pass(struct lens_data *filter, gs_texrender_t *target,
                  gs_technique_t *tech, gs_texture_t *source,
                  gs_texture_t *secondary, uint32_t source_width,
                  uint32_t source_height, uint32_t output_width,
                  uint32_t output_height) {
  if (!target || !tech || !source || source_width == 0 || source_height == 0 ||
      output_width == 0 || output_height == 0)
    return NULL;

  gs_texrender_reset(target);
  if (!gs_texrender_begin(target, output_width, output_height))
    return NULL;

  struct vec4 clear_color = {0};
  gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);

  bind_common_parameters(filter, source, output_width, output_height);
  // Keep the same render contract as the other strategies: common binding can
  // disturb effect parameter state on some backends, so restore the active
  // bloom controls before executing each pyramid pass.
  rebind_current_settings(filter);

  // Sampling offsets belong to the input texture, not the output target.
  // This matters for the downsample chain where those dimensions differ.
  struct vec4 source_pixel_interval;
  vec4_set(&source_pixel_interval, 1.0f / (float)source_width,
           1.0f / (float)source_height, 0.0f, 0.0f);
  if (filter->common.uv_pixel_interval)
    SET_PARAM_VEC4(filter->common.uv_pixel_interval, &source_pixel_interval);

  if (secondary && filter->common.blurred_image)
    SET_PARAM_TEXTURE(filter->common.blurred_image, secondary);

  bool pass_ok =
      lens_run_technique_quad(tech, output_width, output_height);
  gs_texrender_end(target);
  if (!pass_ok)
    return NULL;

  return gs_texrender_get_texture(target);
}

static gs_texture_t *
bloom_build_pyramid(struct lens_data *filter, gs_texture_t *extracted,
                    uint32_t width, uint32_t height) {
  gs_texture_t *current = extracted;
  uint32_t source_width = width;
  uint32_t source_height = height;
  uint32_t level_width[BLOOM_PYRAMID_LEVELS];
  uint32_t level_height[BLOOM_PYRAMID_LEVELS];

  for (size_t i = 0; i < BLOOM_PYRAMID_LEVELS; i++) {
    level_width[i] = bloom_half_dim(source_width);
    level_height[i] = bloom_half_dim(source_height);
    current = bloom_render_pass(
        filter, filter->bloom_pyramid[i], filter->tech.bloom_downsample,
        current, NULL, source_width, source_height, level_width[i],
        level_height[i]);
    if (!current)
      return NULL;

    source_width = level_width[i];
    source_height = level_height[i];
  }

  // Reconstruct from the smallest level. Each pass adds the accumulated
  // lower-resolution glow to the next larger level, matching the pyramid
  // structure used by Alpha-Piscium while staying within OBS effect passes.
  for (int i = BLOOM_PYRAMID_LEVELS - 2; i >= 0; i--) {
    gs_texture_t *base = gs_texrender_get_texture(filter->bloom_pyramid[i]);
    if (!base)
      return NULL;

    current = bloom_render_pass(
        filter, filter->bloom_upsample[i], filter->tech.bloom_upsample, base,
        current, level_width[i], level_height[i], level_width[i],
        level_height[i]);
    if (!current)
      return NULL;
  }

  return current;
}

/**
 * Bloom render - "Dusty Lens + Light Capture" mode.
 *
 * This is intentionally NOT a clean cinematic bloom.
 * It emphasizes how light interacts with dust, haze, and optical imperfections
 * inside a lens: dust scattering, veiling glare (haze), chromatic refraction,
 * and ghost reflections.
 */
gs_texture_t *bloom_render(struct lens_data *filter, gs_effect_t *effect,
                           gs_texture_t *source_tex, uint32_t width,
                           uint32_t height) {
  UNUSED_PARAMETER(effect);

  gs_texrender_t *extract_rt = filter->texrenders[BLOOM_EXTRACT_SLOT];
  if (!extract_rt) {
    EMULENS_LOG(LOG_WARNING, "bloom: extract texrender missing");
    return NULL;
  }

  gs_texrender_reset(extract_rt);
  if (!gs_texrender_begin(extract_rt, width, height)) {
    EMULENS_LOG(LOG_DEBUG, "bloom: extract texrender begin failed");
    return NULL;
  }

  struct vec4 extract_clear_color = {0};
  gs_clear(GS_CLEAR_COLOR, &extract_clear_color, 0.0f, 0);

  bind_common_parameters(filter, source_tex, width, height);
  // Multipass effects must restore mode-specific uniforms after each common
  // bind. Some OBS/OpenGL paths flush effect parameter state during matrix /
  // common texture setup; relying only on lens_render's pre-render update
  // leaves later Bloom techniques running with defaults or stale values.
  rebind_current_settings(filter);

  bool pass_ok = lens_run_technique_quad(filter->tech.bloom_extract, width, height);
  gs_texrender_end(extract_rt);
  if (!pass_ok)
    return source_tex;

  gs_texture_t *extracted = gs_texrender_get_texture(extract_rt);
  if (!extracted) {
    EMULENS_LOG(LOG_WARNING, "bloom: extract pass returned no texture");
    return NULL;
  }

  bool is_anamorphic = filter->runtime_state.bloom.anamorphic;
  float r = filter->runtime_state.bloom.radius;

  gs_texture_t *blurred = NULL;

  if (is_anamorphic) {
    if (!filter->bloom_anam_h || !filter->bloom_anam_v) {
      EMULENS_LOG(LOG_WARNING, "bloom: anamorphic buffers missing");
      return NULL;
    }

    // Horizontal pass
    gs_texrender_reset(filter->bloom_anam_h);
    if (!gs_texrender_begin(filter->bloom_anam_h, width, height)) {
      EMULENS_LOG(LOG_DEBUG, "bloom: anamorphic horizontal begin failed");
      return NULL;
    }
    bind_common_parameters(filter, extracted, width, height);
    rebind_current_settings(filter);

    SET_PARAM_FLOAT(filter->common.blur_radius, r * 2.5f);
    pass_ok = lens_run_technique_quad(filter->tech.blur_h, width, height);
    gs_texrender_end(filter->bloom_anam_h);
    if (!pass_ok)
      return extracted;

    gs_texture_t *h_result = gs_texrender_get_texture(filter->bloom_anam_h);
    if (!h_result) {
      EMULENS_LOG(LOG_WARNING, "bloom: anamorphic horizontal produced no texture");
      return NULL;
    }

    // Vertical pass: scale radius to preserve anamorphic streak at low r
    float v_radius = fmaxf(r * 0.08f, MIN_BLUR_RADIUS);
    gs_texrender_reset(filter->bloom_anam_v);
    if (!gs_texrender_begin(filter->bloom_anam_v, width, height)) {
      EMULENS_LOG(LOG_DEBUG, "bloom: anamorphic vertical begin failed");
      return NULL;
    }
    bind_common_parameters(filter, h_result, width, height);
    rebind_current_settings(filter);

    SET_PARAM_FLOAT(filter->common.blur_radius, v_radius);
    pass_ok = lens_run_technique_quad(filter->tech.blur_v, width, height);
    gs_texrender_end(filter->bloom_anam_v);
    if (!pass_ok)
      return extracted;

    blurred = gs_texrender_get_texture(filter->bloom_anam_v);
  } else {
    blurred = bloom_build_pyramid(filter, extracted, width, height);
    if (!blurred) {
      EMULENS_LOG_WARN(
          "bloom: pyramid failed - falling back to separable Gaussian blur");
      blurred = apply_gaussian_blur(filter, extracted, width, height, r);
    }
  }

  if (!blurred) {
    EMULENS_LOG(LOG_WARNING, "bloom: blur step returned no texture - falling back to extracted");
    blurred = extracted;   // graceful fallback so combine pass still runs
  }

  // Final combine pass - this is where the dusty lens / light capture magic happens
  gs_texrender_t *output_buf = filter->texrenders[BLOOM_OUTPUT_SLOT];
  if (!output_buf) {
    EMULENS_LOG(LOG_WARNING, "bloom: output texrender missing");
    return NULL;
  }

  gs_texrender_reset(output_buf);
  if (!gs_texrender_begin(output_buf, width, height)) {
    EMULENS_LOG(LOG_DEBUG, "bloom: combine texrender begin failed");
    return NULL;
  }

  struct vec4 combine_clear_color = {0};
  gs_clear(GS_CLEAR_COLOR, &combine_clear_color, 0.0f, 0);

  bind_common_parameters(filter, source_tex, width, height);
  rebind_current_settings(filter);

  SET_PARAM_TEXTURE(filter->common.blurred_image, blurred);

  pass_ok = lens_run_technique_quad(filter->tech.bloom_combine, width, height);
  gs_texrender_end(output_buf);
  if (!pass_ok)
    return source_tex;

  gs_texture_t *output_tex = gs_texrender_get_texture(output_buf);
  return output_tex;
}

void bloom_get_defaults(obs_data_t *settings) {
  // Clean highlight bloom first; lens dirt, haze, refraction and ghosts are
  // supporting artifacts and stay near-zero in the neutral baseline.
  obs_data_set_default_double(settings, P_BLOOM_THRESHOLD, 0.72);
  obs_data_set_default_double(settings, P_BLOOM_INTENSITY, 0.60);
  obs_data_set_default_double(settings, P_BLOOM_RADIUS, 18.0);
  obs_data_set_default_double(settings, P_BLOOM_SOFTNESS, 0.24);
  obs_data_set_default_double(settings, P_BLOOM_DUST_AMOUNT, 0.02);
  obs_data_set_default_double(settings, P_BLOOM_DUST_SCALE, 12.0);
  obs_data_set_default_double(settings, P_BLOOM_HAZE_AMOUNT, 0.025);
  obs_data_set_default_double(settings, P_BLOOM_DUST_CONTRAST, 1.0);
  obs_data_set_default_double(settings, P_BLOOM_REFRACT_AMOUNT, 0.0);
  obs_data_set_default_double(settings, P_BLOOM_REFRACT_SCALE, 1.5);
  obs_data_set_default_bool(settings, P_BLOOM_HIGHLIGHTS_ONLY, false);
  obs_data_set_default_bool(settings, P_BLOOM_ANAMORPHIC, false);
  obs_data_set_default_int(settings, P_BLOOM_COLOR, 0xFFFFFFFF);
  obs_data_set_default_double(settings, P_BLOOM_DUST_SPECULAR, 0.20);
  obs_data_set_default_double(settings, P_BLOOM_DUST_DEPTH_LAYERS, 2.0);
  obs_data_set_default_double(settings, P_BLOOM_GHOST_AMOUNT, 0.0);
  obs_data_set_default_double(settings, P_BLOOM_GHOST_SCALE, 2.0);
  obs_data_set_default_int(settings, P_BLOOM_GHOST_COLOR, 0xFFFFFFFF);
}

void bloom_populate_presets(obs_property_t *list) {
  ADD_PRESET_ENTRY(list, "BloomDefault", "default_bloom",
                    "BloomDefaultDesc");
  ADD_PRESET_ENTRY(list, "BloomSoft", "bloom_soft", "BloomSoftDesc");
  ADD_PRESET_ENTRY(list, "BloomDusty", "bloom_dusty_lens", "BloomDustyDesc");
  ADD_PRESET_ENTRY(list, "BloomCyber", "bloom_cyberpunk", "BloomCyberDesc");
  ADD_PRESET_ENTRY(list, "BloomPrism", "bloom_prism", "BloomPrismDesc");
}
