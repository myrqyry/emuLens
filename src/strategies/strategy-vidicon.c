#include "../obs-utils.h"
#include <math.h>

void vidicon_cache(struct lens_data *filter, gs_effect_t *effect) {
  filter->runtime_state.vidicon.time_initialized = false;
  GET_PARAM_SAFE(filter->mode_params.vidicon.history_image, effect,
                 P_VID_HISTORY);
  GET_PARAM_SAFE(filter->mode_params.vidicon.vid_blend, effect, P_VID_BLEND);
  GET_PARAM_SAFE(filter->mode_params.vidicon.vid_color, effect, P_VID_COLOR);
  GET_PARAM_SAFE(filter->mode_params.vidicon.vid_decay, effect, P_VID_DECAY);
  GET_PARAM_SAFE(filter->mode_params.vidicon.vid_threshold, effect,
                 P_VID_THRESHOLD);
  GET_PARAM_SAFE(filter->mode_params.vidicon.vid_red_lag, effect, P_VID_RED_LAG);
  GET_PARAM_SAFE(filter->mode_params.vidicon.vid_blue_lag, effect, P_VID_BLUE_LAG);
  GET_PARAM_SAFE(filter->mode_params.vidicon.vid_comet, effect, P_VID_COMET);
  GET_PARAM_SAFE(filter->mode_params.vidicon.vid_comet_length, effect,
                 P_VID_COMET_LENGTH);
  GET_PARAM_SAFE(filter->mode_params.vidicon.vid_noise, effect, P_VID_NOISE);
  GET_PARAM_SAFE(filter->mode_params.vidicon.vid_beam_pull, effect,
                 P_VID_BEAM_PULL);
  GET_PARAM_SAFE(filter->mode_params.vidicon.vid_lag_nonlinear, effect,
                 P_VID_LAG_NONLINEAR);
  GET_PARAM_SAFE(filter->mode_params.vidicon.vid_register, effect,
                 P_VID_REGISTER);
  GET_PARAM_SAFE(filter->mode_params.vidicon.vid_register_drift, effect,
                 P_VID_REGISTER_DRIFT);
  GET_PARAM_SAFE(filter->mode_params.vidicon.vid_scanlines, effect,
                  P_VID_SCANLINES);
  GET_PARAM_SAFE(filter->mode_params.vidicon.vid_gamma, effect, P_VID_GAMMA);
  GET_PARAM_SAFE(filter->mode_params.vidicon.smear_enable, effect,
                 P_VID_SMEAR_ENABLE);
  GET_PARAM_SAFE(filter->mode_params.vidicon.smear_mode, effect,
                 P_VID_SMEAR_MODE);
  GET_PARAM_SAFE(filter->mode_params.vidicon.smear_direction, effect,
                 P_VID_SMEAR_DIRECTION);
  GET_PARAM_SAFE(filter->mode_params.vidicon.smear_strength, effect,
                  P_VID_SMEAR_STRENGTH);
  GET_PARAM_SAFE(filter->mode_params.vidicon.vid_halation, effect,
                  P_VID_HALATION);
  GET_PARAM_SAFE(filter->mode_params.vidicon.vid_hum, effect,
                  P_VID_HUM);
  GET_PARAM_SAFE(filter->mode_params.vidicon.vid_source_color, effect,
                  P_VID_SOURCE_COLOR);
}

static float vidicon_decay_half_life_seconds(float control) {
  control = clamp_float(control, 0.0f, 1.0f);

  /*
   * Trail Decay is an artistic persistence control, not a raw per-frame
   * multiplier.  The previous direct mapping crushed almost all useful
   * durations into 0.98-0.99, then jumped from a short trail at 0.99 to
   * permanent history at 1.00.
   *
   * These anchors spread perceptually distinct tube-persistence durations
   * across the slider while preserving 1.00 as the intentional hold-forever
   * endpoint.
   */
  if (control <= 0.0f)
    return 0.0f;
  if (control <= 0.50f)
    return 0.02f + (0.18f - 0.02f) * (control / 0.50f);
  if (control <= 0.80f)
    return 0.18f + (0.60f - 0.18f) * ((control - 0.50f) / 0.30f);
  if (control <= 0.90f)
    return 0.60f + (1.20f - 0.60f) * ((control - 0.80f) / 0.10f);
  if (control <= 0.95f)
    return 1.20f + (2.50f - 1.20f) * ((control - 0.90f) / 0.05f);
  if (control <= 0.99f)
    return 2.50f + (6.00f - 2.50f) * ((control - 0.95f) / 0.04f);

  // Fractional values between the UI's 0.99 and 1.00 steps remain finite.
  return 6.00f + (20.0f - 6.00f) * ((control - 0.99f) / 0.01f);
}

static float vidicon_nominal_60hz_decay(float control) {
  if (control <= 0.0f)
    return 0.0f;
  if (control >= 0.9995f)
    return 1.0f;

  float half_life = vidicon_decay_half_life_seconds(control);
  if (half_life <= 0.0f)
    return 0.0f;

  return exp2f(-(1.0f / 60.0f) / half_life);
}

void vidicon_update(struct lens_data *filter, obs_data_t *settings) {
  filter->runtime_state.vidicon.user_decay = clamp_float(
      (float)obs_data_get_double(settings, P_VID_DECAY), 0.0f, 1.0f);
  SET_PARAM_FLOAT(filter->mode_params.vidicon.vid_threshold, clamp_float(
      (float)obs_data_get_double(settings, P_VID_THRESHOLD), 0.0f, 1.0f));
  uint32_t c_val = (uint32_t)obs_data_get_int(settings, P_VID_COLOR);
  struct vec4 _c_vec_color;
  emulens_color_to_vec4(&_c_vec_color, c_val);
  SET_PARAM_VEC4(filter->mode_params.vidicon.vid_color, &_c_vec_color);
  SET_PARAM_FLOAT(filter->mode_params.vidicon.vid_blend, clamp_float(
      (float)obs_data_get_double(settings, P_VID_BLEND), 0.0f, 1.0f));
  SET_PARAM_FLOAT(filter->mode_params.vidicon.vid_red_lag, clamp_float(
      (float)obs_data_get_double(settings, P_VID_RED_LAG), 0.0f, 1.0f));
  SET_PARAM_FLOAT(filter->mode_params.vidicon.vid_blue_lag, clamp_float(
      (float)obs_data_get_double(settings, P_VID_BLUE_LAG), 0.0f, 1.0f));
  bool comet_on = obs_data_get_bool(settings, P_VID_COMET);
  SET_PARAM_FLOAT(filter->mode_params.vidicon.vid_comet, comet_on ? 1.0f : 0.0f);
  SET_PARAM_FLOAT(filter->mode_params.vidicon.vid_comet_length, clamp_float(
      (float)obs_data_get_double(settings, P_VID_COMET_LENGTH), 1.0f, 20.0f));
  SET_PARAM_FLOAT(filter->mode_params.vidicon.vid_noise, clamp_float(
      (float)obs_data_get_double(settings, P_VID_NOISE), 0.0f, 1.0f));
  SET_PARAM_FLOAT(filter->mode_params.vidicon.vid_beam_pull, clamp_float(
      (float)obs_data_get_double(settings, P_VID_BEAM_PULL), 0.0f, 1.0f));
  SET_PARAM_FLOAT(filter->mode_params.vidicon.vid_lag_nonlinear, clamp_float(
      (float)obs_data_get_double(settings, P_VID_LAG_NONLINEAR), 0.0f, 1.0f));
  SET_PARAM_FLOAT(filter->mode_params.vidicon.vid_register, clamp_float(
      (float)obs_data_get_double(settings, P_VID_REGISTER), 0.0f, 10.0f));
  SET_PARAM_FLOAT(filter->mode_params.vidicon.vid_register_drift, clamp_float(
      (float)obs_data_get_double(settings, P_VID_REGISTER_DRIFT), 0.0f, 5.0f));
  SET_PARAM_FLOAT(filter->mode_params.vidicon.vid_scanlines, clamp_float(
      (float)obs_data_get_double(settings, P_VID_SCANLINES), 0.0f, 1.0f));
  SET_PARAM_FLOAT(filter->mode_params.vidicon.vid_gamma, clamp_float(
      (float)obs_data_get_double(settings, P_VID_GAMMA), 0.3f, 2.0f));

  SET_PARAM_FLOAT(filter->mode_params.vidicon.smear_enable,
                   obs_data_get_bool(settings, P_VID_SMEAR_ENABLE) ? 1.0f : 0.0f);
  SET_CLAMPED_INT_AS_FLOAT(filter->mode_params.vidicon.smear_mode,
                           settings, P_VID_SMEAR_MODE, 0.0f, 3.0f);
  SET_CLAMPED_INT_AS_FLOAT(filter->mode_params.vidicon.smear_direction,
                           settings, P_VID_SMEAR_DIRECTION, 0.0f, 3.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.vidicon.smear_strength,
                     settings, P_VID_SMEAR_STRENGTH, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.vidicon.vid_halation,
                     settings, P_VID_HALATION, 0.0f, 1.0f);
  SET_CLAMPED_DOUBLE(filter->mode_params.vidicon.vid_hum,
                     settings, P_VID_HUM, 0.0f, 1.0f);
  SET_PARAM_FLOAT(filter->mode_params.vidicon.vid_source_color,
                   obs_data_get_bool(settings, P_VID_SOURCE_COLOR) ? 1.0f : 0.0f);
}

/**
 * Vidicon render with improved history ping-pong handling.
 *
 * We maintain two history buffers and flip cur_write after each successful frame.
 * This creates a feedback loop for trails. The key is ensuring we always read
 * from the *previous* frame's result and write into the other buffer.
 */
gs_texture_t *vidicon_render(struct lens_data *filter, gs_effect_t *effect,
                             gs_texture_t *source_tex, uint32_t width,
                             uint32_t height) {
  UNUSED_PARAMETER(effect);
  int cur_write = emulens_atomic_load(&filter->cur_write);
#ifdef EMULENS_VERBOSE_DEBUG
  EMULENS_LOG(LOG_DEBUG, "vidicon: render entry (%ux%u) cur_write=%d",
              width, height, cur_write);
#endif

  // Safety clamp on cur_write to prevent any possibility of out-of-bounds access
  if (cur_write < 0 || cur_write > 1) {
    EMULENS_LOG_WARN("vidicon: cur_write out of range (%d) — resetting to 0", cur_write);
    cur_write = 0;
    emulens_atomic_store(&filter->cur_write, cur_write);
  }

  // history_buf = the buffer we will READ from (previous frame's trails)
  // trail_buf  = the buffer we will WRITE into (new trails)
  int read_idx  = (cur_write + 1) % 2;
  int write_idx = cur_write;

  gs_texrender_t *history_buf = filter->vidicon_history[read_idx];
  gs_texrender_t *trail_buf   = filter->vidicon_history[write_idx];

  if (!history_buf || !trail_buf || !filter->output_texrender) {
    EMULENS_LOG(LOG_WARNING, "vidicon: missing texrender buffers");
    return NULL;
  }

  // === PASS 1: Generate new trails ===
  gs_texrender_reset(trail_buf);
  if (!gs_texrender_begin(trail_buf, width, height)) {
    EMULENS_LOG(LOG_DEBUG, "vidicon: trail texrender begin failed");
    return NULL;
  }

  struct vec4 trail_clear_color = {0};
  gs_clear(GS_CLEAR_COLOR, &trail_clear_color, 0.0f, 0);

  bind_common_parameters(filter, source_tex, width, height);
  rebind_current_settings(filter);

  // Tube persistence is a function of elapsed time, not render-call count.
  // A source can be rendered multiple times in one video tick (preview,
  // projector, screenshot, etc.). Normalizing to a nominal 60 Hz preserves the
  // old slider feel while duplicate renders in the same tick consume no extra
  // persistence.
  float elapsed_sec =
      (float)emulens_atomic_load(&filter->elapsed_time_ns) / 1000000000.0f;
  float dt = 1.0f / 60.0f;
  if (filter->runtime_state.vidicon.time_initialized) {
    dt = elapsed_sec - filter->runtime_state.vidicon.prev_elapsed_sec;
    if (dt < 0.0f)
      dt += TIME_WRAP_THRESHOLD;
    dt = clamp_float(dt, 0.0f, 0.25f);
  } else {
    filter->runtime_state.vidicon.time_initialized = true;
  }
  filter->runtime_state.vidicon.prev_elapsed_sec = elapsed_sec;

  float user_decay = filter->runtime_state.vidicon.user_decay;
  float frame_decay = 0.0f;
  if (user_decay >= 0.001f) {
    float nominal_60hz_decay = vidicon_nominal_60hz_decay(user_decay);
    frame_decay = powf(nominal_60hz_decay, dt * 60.0f);
  }
  SET_PARAM_FLOAT(filter->mode_params.vidicon.vid_decay, frame_decay);

  // ponytail: rebind_current_settings removed — lens_render already calls
  // strat->update_settings before strat->render, so settings are already bound.

  // Bind the PREVIOUS frame's history as input for feedback
  if (filter->mode_params.vidicon.history_image) {
    gs_texture_t *prev_history = gs_texrender_get_texture(history_buf);
    SET_PARAM_TEXTURE(filter->mode_params.vidicon.history_image,
                      prev_history ? prev_history : source_tex);
  }

  bool pass_ok = lens_run_technique_quad(filter->tech.vidicon, width, height);
  gs_texrender_end(trail_buf);
  if (!pass_ok)
    return source_tex;

  gs_texture_t *new_trails = gs_texrender_get_texture(trail_buf);
  if (!new_trails) {
    EMULENS_LOG(LOG_WARNING, "vidicon: failed to get trail texture");
    return NULL;
  }

  // === PASS 2: Composite final look ===
  gs_texrender_reset(filter->output_texrender);
  if (!gs_texrender_begin(filter->output_texrender, width, height)) {
    EMULENS_LOG(LOG_DEBUG, "vidicon: composite texrender begin failed");
    return NULL;
  }

  struct vec4 composite_clear_color = {0};
  gs_clear(GS_CLEAR_COLOR, &composite_clear_color, 0.0f, 0);

  bind_common_parameters(filter, source_tex, width, height);
  rebind_current_settings(filter);

  // Now bind the NEW trails we just generated for the composite pass
  SET_PARAM_TEXTURE(filter->mode_params.vidicon.history_image, new_trails);

  pass_ok = lens_run_technique_quad(filter->tech.vidicon_composite, width, height);
  gs_texrender_end(filter->output_texrender);
  if (!pass_ok)
    return source_tex;

  // Flip for next frame
  emulens_atomic_store(&filter->cur_write, (cur_write + 1) % 2);

  gs_texture_t *output_tex = gs_texrender_get_texture(filter->output_texrender);
#ifdef EMULENS_VERBOSE_DEBUG
  EMULENS_LOG(LOG_DEBUG, "vidicon: render exit cur_write=%d texture=%p",
              (cur_write + 1) % 2, (void *)output_tex);
#endif
  return output_tex;
}

void vidicon_destroy_cpu(struct lens_data *filter) {
  if (!filter)
    return;
  filter->runtime_state.vidicon.user_decay = 0.0f;
  filter->runtime_state.vidicon.prev_elapsed_sec = 0.0f;
  filter->runtime_state.vidicon.time_initialized = false;
}
void vidicon_destroy(struct lens_data *filter) { (void)filter; }

void vidicon_get_defaults(obs_data_t *settings) {
  // Tube persistence is stored as trail energy, not whole-frame feedback.
  // This lets decay live in a realistic high range without pumping exposure.
  obs_data_set_default_double(settings, P_VID_DECAY, 0.80);
  obs_data_set_default_double(settings, P_VID_THRESHOLD, 0.64);
  obs_data_set_default_int(settings, P_VID_COLOR, 0xFFFFFFFF);
  obs_data_set_default_double(settings, P_VID_BLEND, 0.0);
  obs_data_set_default_double(settings, P_VID_RED_LAG, 0.28);
  obs_data_set_default_double(settings, P_VID_BLUE_LAG, 0.10);
  obs_data_set_default_bool(settings, P_VID_COMET, false);
  obs_data_set_default_double(settings, P_VID_COMET_LENGTH, 4.0);
  obs_data_set_default_double(settings, P_VID_NOISE, 0.012);
  obs_data_set_default_double(settings, P_VID_BEAM_PULL, 0.0);
  obs_data_set_default_double(settings, P_VID_LAG_NONLINEAR, 0.12);
  obs_data_set_default_double(settings, P_VID_REGISTER, 0.02);
  obs_data_set_default_double(settings, P_VID_REGISTER_DRIFT, 0.01);
  obs_data_set_default_double(settings, P_VID_SCANLINES, 0.008);
  obs_data_set_default_double(settings, P_VID_GAMMA, 1.0);
  obs_data_set_default_double(settings, P_VID_HALATION, 0.035);
  obs_data_set_default_double(settings, P_VID_HUM, 0.006);
  obs_data_set_default_bool(settings, P_VID_SOURCE_COLOR, true);
  obs_data_set_default_bool(settings, P_VID_SMEAR_ENABLE, false);
  obs_data_set_default_int(settings, P_VID_SMEAR_MODE, 0);
  obs_data_set_default_int(settings, P_VID_SMEAR_DIRECTION, 0);
  obs_data_set_default_double(settings, P_VID_SMEAR_STRENGTH, 0.0);
}

void vidicon_populate_presets(obs_property_t *list) {
  ADD_PRESET_ENTRY(list, "VidiconDefault", "default_vidicon",
                    "VidiconDefaultDesc");
  ADD_PRESET_ENTRY(list, "VidiconDreamy", "vidicon_dream",
                    "VidiconDreamyDesc");
  ADD_PRESET_ENTRY(list, "VidiconSoft", "vidicon_soft",
                    "VidiconSoftDesc");
}
