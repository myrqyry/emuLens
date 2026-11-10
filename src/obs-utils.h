/**
 * @file obs-utils.h
 * @brief Shared OBS-specific helper layer.
 *
 * Reimplements the helper patterns from obs-retro-effects (FiniteSingularity,
 * MIT-style architecture reference) without copying any of its source. The
 * patterns encoded here are the OBS-idiomatic way to:
 *   - capture the parent source into a texrender with the right color space
 *   - composite the final output back to OBS with the right color space
 *   - set/clear the render state for an effect pass
 *   - toggle a property's visibility + enabled state together
 *
 * Each helper is intentionally small and side-effect-narrow so strategies
 * can compose them in any order.
 */
#pragma once

#include "lens-data.h"
#include <obs-module.h>

/**
 * Toggle both `obs_property_set_visible` and `obs_property_set_enabled`
 * in one call. Null-safe on the property lookup. Used by lens-ui.c
 * bool-gated visibility callbacks.
 */
void setting_visibility(const char *prop_name, bool visible,
                        obs_properties_t *props);

/**
 * Push blend state and reset to a known "off" configuration:
 *   gs_blend_state_push();
 *   gs_reset_blend_state();
 *   gs_enable_blending(false);
 *   gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
 *
 * Caller is responsible for the matching gs_blend_state_pop().
 */
void set_blending_parameters(void);

/**
 * Capture the upstream source's current frame into
 * `filter->input_texrender`.
 *
 * This filter uses OBS_SOURCE_CUSTOM_DRAW, so we manage our own
 * texrender pipeline. The upstream source is rendered directly into
 * input_texrender at the given dimensions. Color-space propagation
 * is handled via the video_get_color_space callback on the filter
 * info struct (see lens_get_color_space in lens-core.c), which
 * forwards to the parent source.
 *
 * Returns true if the input texture was captured; false if the
 * filter context is invalid or the texrender begin failed.
 */
bool get_input_source(struct lens_data *filter, uint32_t width,
                      uint32_t height);

void rebind_current_settings(struct lens_data *filter);

/**
 * Read an effect file and recursively inline quoted relative #include
 * directives. The returned buffer is allocated with OBS's allocator and must
 * be released with bfree().
 */
char *load_shader_from_file(const char *file_path);

// Trivial single-pass render: bind common params, run one technique into
// one texrender slot, return the resulting texture. Returns NULL on
// missing buffer / failed begin / missing technique. Used by every
// strategy whose effect is a single technique applied to the source.
static inline gs_texture_t *
render_passthrough_to_texrender(gs_texrender_t *output_buf,
                                gs_technique_t *tech,
                                struct lens_data *filter,
                                gs_texture_t *source_tex, uint32_t width,
                                uint32_t height) {
  if (!output_buf || !tech)
    return NULL;

  gs_texrender_reset(output_buf);
  if (!gs_texrender_begin(output_buf, width, height))
    return NULL;

  /*
   * Texrender passes must not inherit OBS's scene-composite blend state.
   * A valid shader pass can otherwise execute successfully while writing no
   * visible pixels into a freshly cleared target.
   */
  set_blending_parameters();

  struct vec4 clear_color;
  vec4_set(&clear_color, 0.0f, 0.0f, 0.0f, 0.0f);
  gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);

  bind_common_parameters(filter, source_tex, width, height);
  // No rebind_current_settings needed here: bind_common_parameters only
  // binds common.* params and sets the matrix; mode-specific params were
  // already bound by the strategy's *_update before this passthrough ran.
  bool pass_ok = lens_run_technique_quad(tech, width, height);

  gs_blend_state_pop();
  gs_texrender_end(output_buf);
  if (!pass_ok)
    return NULL;

  return gs_texrender_get_texture(output_buf);
}
