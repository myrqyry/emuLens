#include "../obs-utils.h"

// Multi-octave noise: combine 3 octaves for richer motion (band-limited)
static float handheld_fbm_noise(float x, float y) {
  // Low-freq drift
  float ix = floorf(x);
  float fx = x - ix;
  fx = fx * fx * (3.0f - 2.0f * fx);
  float a = sinf(ix * 12.9898f + y * 78.233f) * 43758.5453f;
  a = a - floorf(a);
  float b = sinf((ix + 1.0f) * 12.9898f + y * 78.233f) * 43758.5453f;
  b = b - floorf(b);
  return a + (b - a) * fx;
}

static float handheld_noise(float x, float y) {
  // Layered noise: low-freq drift + mid-freq walk + high-freq jitter
  float low = handheld_fbm_noise(x, y) * 0.6f;
  float mid = handheld_fbm_noise(x * 3.7f + 5.3f, y * 3.7f + 2.1f) * 0.3f;
  float high = handheld_fbm_noise(x * 11.3f + 17.0f, y * 11.3f + 9.0f) * 0.1f;
  return clamp_float(low + mid + high, 0.0f, 1.0f);
}

#define HANDHELD_DEG_TO_RAD 0.01745329252f
#define HANDHELD_TWO_PI 6.28318530718f

static float handheld_signed_noise(float x, float y) {
  return (handheld_noise(x, y) - 0.5f) * 2.0f;
}

static float handheld_lerp(float a, float b, float t) {
  return a + (b - a) * clamp_float(t, 0.0f, 1.0f);
}

void handheld_cache(struct lens_data *filter, gs_effect_t *effect) {
  GET_PARAM_SAFE(filter->mode_params.handheld.pos_amp, effect, P_HAND_POS_AMP);
  GET_PARAM_SAFE(filter->mode_params.handheld.pos_speed, effect,
                 P_HAND_POS_SPEED);
  GET_PARAM_SAFE(filter->mode_params.handheld.rot_amp, effect, P_HAND_ROT_AMP);
  GET_PARAM_SAFE(filter->mode_params.handheld.rot_speed, effect,
                 P_HAND_ROT_SPEED);
  GET_PARAM_SAFE(filter->mode_params.handheld.zoom_amp, effect,
                 P_HAND_ZOOM_AMP);
  GET_PARAM_SAFE(filter->mode_params.handheld.zoom_speed, effect,
                 P_HAND_ZOOM_SPEED);
  GET_PARAM_SAFE(filter->mode_params.handheld.pos_offset, effect,
                 P_HAND_POS_OFFSET);
  GET_PARAM_SAFE(filter->mode_params.handheld.rot_rad, effect, P_HAND_ROT_RAD);
  GET_PARAM_SAFE(filter->mode_params.handheld.zoom_factor, effect,
                 P_HAND_ZOOM_FACTOR);
  GET_PARAM_SAFE(filter->mode_params.handheld.blur_enable, effect,
                 P_HAND_BLUR_ENABLE);
  GET_PARAM_SAFE(filter->mode_params.handheld.blur_amp, effect,
                 P_HAND_BLUR_AMP);
  GET_PARAM_SAFE(filter->mode_params.handheld.blur_speed, effect,
                 P_HAND_BLUR_SPEED);
  GET_PARAM_SAFE(filter->mode_params.handheld.blur_static, effect,
                 P_HAND_BLUR_STATIC);
  GET_PARAM_SAFE(filter->mode_params.handheld.feather, effect, P_HAND_FEATHER);
  GET_PARAM_SAFE(filter->mode_params.handheld.vel_dir_x, effect,
                 P_HAND_VEL_DIR_X);
  GET_PARAM_SAFE(filter->mode_params.handheld.vel_dir_y, effect,
                 P_HAND_VEL_DIR_Y);
  GET_PARAM_SAFE(filter->mode_params.handheld.vel_mag, effect, P_HAND_VEL_MAG);
  GET_PARAM_SAFE(filter->mode_params.handheld.dir_blur_enable, effect,
                 P_HAND_DIR_BLUR_ENABLE);
  GET_PARAM_SAFE(filter->mode_params.handheld.blur_directional, effect,
                 P_HAND_BLUR_DIRECTIONAL);
}

void handheld_update(struct lens_data *filter, obs_data_t *settings) {
  filter->runtime_state.handheld.pos_amp =
      clamp_float((float)obs_data_get_double(settings, P_HAND_POS_AMP),
                  HAND_POS_AMP_MIN, HAND_POS_AMP_MAX);
  SET_PARAM_FLOAT(filter->mode_params.handheld.pos_amp,
                  filter->runtime_state.handheld.pos_amp);
  filter->runtime_state.handheld.pos_speed =
      clamp_float((float)obs_data_get_double(settings, P_HAND_POS_SPEED),
                  HAND_POS_SPEED_MIN, HAND_POS_SPEED_MAX);
  SET_PARAM_FLOAT(filter->mode_params.handheld.pos_speed,
                  filter->runtime_state.handheld.pos_speed);
  filter->runtime_state.handheld.rot_amp =
      clamp_float((float)obs_data_get_double(settings, P_HAND_ROT_AMP),
                  HAND_ROT_AMP_MIN, HAND_ROT_AMP_MAX);
  SET_PARAM_FLOAT(filter->mode_params.handheld.rot_amp,
                  filter->runtime_state.handheld.rot_amp);
  filter->runtime_state.handheld.rot_speed = clamp_float(
      (float)obs_data_get_double(settings, P_HAND_ROT_SPEED), 0.0f, 5.0f);
  SET_PARAM_FLOAT(filter->mode_params.handheld.rot_speed,
                  filter->runtime_state.handheld.rot_speed);
  filter->runtime_state.handheld.zoom_amp = clamp_float(
      (float)obs_data_get_double(settings, P_HAND_ZOOM_AMP), 0.0f, 0.5f);
  SET_PARAM_FLOAT(filter->mode_params.handheld.zoom_amp,
                  filter->runtime_state.handheld.zoom_amp);
  filter->runtime_state.handheld.zoom_speed = clamp_float(
      (float)obs_data_get_double(settings, P_HAND_ZOOM_SPEED), 0.0f, 5.0f);
  SET_PARAM_FLOAT(filter->mode_params.handheld.zoom_speed,
                  filter->runtime_state.handheld.zoom_speed);
  filter->runtime_state.handheld.blur_enable =
      obs_data_get_bool(settings, P_HAND_BLUR_ENABLE);
  SET_PARAM_FLOAT(filter->mode_params.handheld.blur_enable,
                  filter->runtime_state.handheld.blur_enable ? 1.0f : 0.0f);
  filter->runtime_state.handheld.blur_amp = clamp_float(
      (float)obs_data_get_double(settings, P_HAND_BLUR_AMP), 0.0f, 20.0f);
  SET_PARAM_FLOAT(filter->mode_params.handheld.blur_amp,
                  filter->runtime_state.handheld.blur_amp);
  filter->runtime_state.handheld.blur_speed = clamp_float(
      (float)obs_data_get_double(settings, P_HAND_BLUR_SPEED), 0.0f, 10.0f);
  SET_PARAM_FLOAT(filter->mode_params.handheld.blur_speed,
                  filter->runtime_state.handheld.blur_speed);
  filter->runtime_state.handheld.tremor_amp =
      clamp_float((float)obs_data_get_double(settings, P_HAND_TREMOR_AMP),
                  HAND_TREMOR_AMP_MIN, HAND_TREMOR_AMP_MAX);
  filter->runtime_state.handheld.breath_amp =
      clamp_float((float)obs_data_get_double(settings, P_HAND_BREATH_AMP),
                  HAND_BREATH_AMP_MIN, HAND_BREATH_AMP_MAX);
  filter->runtime_state.handheld.step_amp =
      clamp_float((float)obs_data_get_double(settings, P_HAND_STEP_AMP),
                  HAND_STEP_AMP_MIN, HAND_STEP_AMP_MAX);
  filter->runtime_state.handheld.step_speed =
      clamp_float((float)obs_data_get_double(settings, P_HAND_STEP_SPEED),
                  HAND_STEP_SPEED_MIN, HAND_STEP_SPEED_MAX);
  filter->runtime_state.handheld.correction_amp =
      clamp_float((float)obs_data_get_double(settings, P_HAND_CORRECTION_AMP),
                  HAND_CORRECTION_AMP_MIN, HAND_CORRECTION_AMP_MAX);
  filter->runtime_state.handheld.inertia =
      clamp_float((float)obs_data_get_double(settings, P_HAND_INERTIA),
                  HAND_INERTIA_MIN, HAND_INERTIA_MAX);
  filter->runtime_state.handheld.blur_response =
      clamp_float((float)obs_data_get_double(settings, P_HAND_BLUR_RESPONSE),
                  HAND_BLUR_RESPONSE_MIN, HAND_BLUR_RESPONSE_MAX);
  filter->runtime_state.handheld.blur_static = clamp_float(
      (float)obs_data_get_double(settings, P_HAND_BLUR_STATIC), 0.0f, 20.0f);
  SET_PARAM_FLOAT(filter->mode_params.handheld.blur_static,
                  filter->runtime_state.handheld.blur_static);
  SET_PARAM_FLOAT(
      filter->mode_params.handheld.feather,
      clamp_float((float)obs_data_get_double(settings, P_HAND_FEATHER), 0.0f,
                  1.0f));

  SET_PARAM_FLOAT(filter->mode_params.handheld.vel_dir_x, clamp_float(
      (float)obs_data_get_double(settings, P_HAND_VEL_DIR_X), -1.0f, 1.0f));
  SET_PARAM_FLOAT(filter->mode_params.handheld.vel_dir_y, clamp_float(
      (float)obs_data_get_double(settings, P_HAND_VEL_DIR_Y), -1.0f, 1.0f));
  SET_PARAM_FLOAT(filter->mode_params.handheld.vel_mag, clamp_float(
      (float)obs_data_get_double(settings, P_HAND_VEL_MAG), 0.0f, 1.0f));
  bool dir_blur_on = obs_data_get_bool(settings, P_HAND_DIR_BLUR_ENABLE);
  SET_PARAM_FLOAT(filter->mode_params.handheld.dir_blur_enable,
                  dir_blur_on ? 1.0f : 0.0f);
  bool blur_dir_on = obs_data_get_bool(settings, P_HAND_BLUR_DIRECTIONAL);
  SET_PARAM_FLOAT(filter->mode_params.handheld.blur_directional,
                  blur_dir_on ? 1.0f : 0.0f);
}

gs_texture_t *handheld_render(struct lens_data *filter, gs_effect_t *effect,
                              gs_texture_t *source_tex, uint32_t width,
                              uint32_t height) {
  UNUSED_PARAMETER(effect);

  // Safety: ensure we have a valid output buffer
  gs_texrender_t *output_buf = filter->texrenders[0];
  if (!output_buf) {
    EMULENS_LOG(LOG_WARNING, "handheld: output texrender missing");
    return NULL;
  }

  float pos_amp = filter->runtime_state.handheld.pos_amp;
  float pos_speed = filter->runtime_state.handheld.pos_speed;
  float rot_amp = filter->runtime_state.handheld.rot_amp;
  float rot_speed = filter->runtime_state.handheld.rot_speed;
  float zoom_amp = filter->runtime_state.handheld.zoom_amp;
  float zoom_speed = filter->runtime_state.handheld.zoom_speed;
  float tremor_amp = filter->runtime_state.handheld.tremor_amp;
  float breath_amp = filter->runtime_state.handheld.breath_amp;
  float step_amp = filter->runtime_state.handheld.step_amp;
  float step_speed = filter->runtime_state.handheld.step_speed;
  float correction_amp = filter->runtime_state.handheld.correction_amp;
  float inertia = filter->runtime_state.handheld.inertia;

  float elapsed_sec =
      (float)emulens_atomic_load(&filter->elapsed_time_ns) / 1000000000.0f;
  float phase = filter->runtime_state.handheld.phase_seed;
  float aspect =
      (height != 0 && width != 0) ? ((float)height / (float)width) : 1.0f;

  // CPU motion simulation is kept in place to preserve existing behavior.

  float drift_t = (elapsed_sec + phase) * pos_speed * 0.10f;
  float rot_t = (elapsed_sec + phase * 1.7f) * rot_speed * 0.10f;
  float zoom_t = (elapsed_sec + phase * 0.3f) * zoom_speed * 0.10f;
  float breath_phase = (elapsed_sec + phase * 0.13f) *
                       (0.20f + pos_speed * 0.08f) * HANDHELD_TWO_PI;
  float step_phase =
      (elapsed_sec + phase * 0.07f) * step_speed * HANDHELD_TWO_PI;
  float tremor_t = (elapsed_sec + phase * 0.37f) * (8.0f + pos_speed * 6.0f);
  float correction_t = (elapsed_sec + phase * 0.19f) * 0.08f;

  float drift_x = handheld_signed_noise(drift_t, 5.3f);
  float drift_y = handheld_signed_noise(drift_t + 17.0f, 2.1f);
  float correction_x = handheld_signed_noise(correction_t, 12.4f);
  float correction_y = handheld_signed_noise(correction_t + 31.0f, 4.7f);
  float tremor_x = handheld_signed_noise(tremor_t, 44.1f);
  float tremor_y = handheld_signed_noise(tremor_t + 9.0f, 19.6f);
  float step_wave = sinf(step_phase);
  float step_impact = fabsf(step_wave);
  float breath = sinf(breath_phase);

  struct vec2 target_offset;
  float target_x = drift_x * pos_amp;
  target_x += correction_x * pos_amp * correction_amp * 0.75f;
  target_x += sinf(step_phase * 0.5f) * pos_amp * step_amp * 0.60f;
  target_x += tremor_x * pos_amp * tremor_amp * 0.12f;

  float target_y = drift_y * pos_amp;
  target_y += breath * pos_amp * breath_amp * 0.60f;
  target_y += step_impact * pos_amp * step_amp * 0.80f;
  target_y += correction_y * pos_amp * correction_amp * 0.50f;
  target_y += tremor_y * pos_amp * tremor_amp * 0.12f;
  vec2_set(&target_offset, target_x / aspect, target_y);

  float target_rot_deg = handheld_signed_noise(rot_t, 9.8f) * rot_amp;
  target_rot_deg += step_wave * rot_amp * step_amp * 0.30f;
  target_rot_deg += tremor_x * rot_amp * tremor_amp * 0.08f;
  target_rot_deg += correction_x * rot_amp * correction_amp * 0.25f;
  float target_rot_rad = target_rot_deg * HANDHELD_DEG_TO_RAD;

  float target_zoom = 1.0f + handheld_signed_noise(17.2f, zoom_t) * zoom_amp;
  target_zoom += breath * zoom_amp * breath_amp * 0.80f;
  target_zoom -= step_impact * zoom_amp * step_amp * 0.50f;
  target_zoom += correction_y * zoom_amp * correction_amp * 0.40f;
  target_zoom = clamp_float(target_zoom, 0.1f, 4.0f);

  float dt = 1.0f / 60.0f;
  if (filter->runtime_state.handheld.motion_initialized) {
    dt = elapsed_sec - filter->runtime_state.handheld.prev_elapsed_sec;
    if (dt <= 0.0001f || dt > 0.25f)
      dt = 1.0f / 60.0f;
  }

  struct vec2 previous_offset = filter->runtime_state.handheld.smoothed_offset;
  float previous_rot_rad = filter->runtime_state.handheld.smoothed_rot_rad;
  float previous_zoom_factor =
      filter->runtime_state.handheld.smoothed_zoom_factor;

  if (!filter->runtime_state.handheld.motion_initialized) {
    filter->runtime_state.handheld.smoothed_offset = target_offset;
    filter->runtime_state.handheld.prev_offset = target_offset;
    filter->runtime_state.handheld.smoothed_rot_rad = target_rot_rad;
    filter->runtime_state.handheld.prev_rot_rad = target_rot_rad;
    filter->runtime_state.handheld.smoothed_zoom_factor = target_zoom;
    filter->runtime_state.handheld.prev_zoom_factor = target_zoom;
    filter->runtime_state.handheld.prev_elapsed_sec = elapsed_sec;
    filter->runtime_state.handheld.motion_initialized = true;
    previous_offset = target_offset;
    previous_rot_rad = target_rot_rad;
    previous_zoom_factor = target_zoom;
  } else {
    float responsiveness = 2.0f + (1.0f - inertia) * 28.0f;
    float smooth_t = clamp_float(1.0f - expf(-responsiveness * dt), 0.0f, 1.0f);
    filter->runtime_state.handheld.prev_offset = previous_offset;
    filter->runtime_state.handheld.prev_rot_rad = previous_rot_rad;
    filter->runtime_state.handheld.prev_zoom_factor = previous_zoom_factor;
    filter->runtime_state.handheld.smoothed_offset.x =
        handheld_lerp(filter->runtime_state.handheld.smoothed_offset.x,
                      target_offset.x, smooth_t);
    filter->runtime_state.handheld.smoothed_offset.y =
        handheld_lerp(filter->runtime_state.handheld.smoothed_offset.y,
                      target_offset.y, smooth_t);
    filter->runtime_state.handheld.smoothed_rot_rad =
        handheld_lerp(filter->runtime_state.handheld.smoothed_rot_rad,
                      target_rot_rad, smooth_t);
    filter->runtime_state.handheld.smoothed_zoom_factor =
        handheld_lerp(filter->runtime_state.handheld.smoothed_zoom_factor,
                      target_zoom, smooth_t);
    filter->runtime_state.handheld.prev_elapsed_sec = elapsed_sec;
  }

  struct vec2 pos_offset = filter->runtime_state.handheld.smoothed_offset;
  float rot_rad = filter->runtime_state.handheld.smoothed_rot_rad;
  float zoom_factor = filter->runtime_state.handheld.smoothed_zoom_factor;

  if (filter->mode_params.handheld.pos_offset)
    SET_PARAM_VEC2(filter->mode_params.handheld.pos_offset, &pos_offset);
  if (filter->mode_params.handheld.rot_rad)
    SET_PARAM_FLOAT(filter->mode_params.handheld.rot_rad, rot_rad);
  if (filter->mode_params.handheld.zoom_factor)
    SET_PARAM_FLOAT(filter->mode_params.handheld.zoom_factor, zoom_factor);

  float inv_dt = (dt > 0.0001f) ? (1.0f / dt) : 60.0f;
  float dx = (pos_offset.x - previous_offset.x) * aspect;
  float dy = pos_offset.y - previous_offset.y;
  float offset_velocity = sqrtf(dx * dx + dy * dy) * inv_dt * 120.0f;
  float rot_velocity = fabsf(rot_rad - previous_rot_rad) * inv_dt * 18.0f;
  float zoom_velocity =
      fabsf(zoom_factor - previous_zoom_factor) * inv_dt * 12.0f;
  float motion_velocity = offset_velocity + rot_velocity + zoom_velocity;

  float blur_r = filter->runtime_state.handheld.blur_static;
  if (filter->runtime_state.handheld.blur_enable &&
      filter->runtime_state.handheld.blur_amp > 0.001f) {
    float blur_smooth_t = clamp_float(
        dt * (1.0f + filter->runtime_state.handheld.blur_speed * 4.0f), 0.0f,
        1.0f);
    filter->runtime_state.handheld.blur_velocity =
        handheld_lerp(filter->runtime_state.handheld.blur_velocity,
                      motion_velocity, blur_smooth_t);
    float dynamic_blur =
        filter->runtime_state.handheld.blur_amp *
        clamp_float(filter->runtime_state.handheld.blur_velocity *
                        filter->runtime_state.handheld.blur_response,
                    0.0f, 1.0f);
    blur_r += dynamic_blur;
  } else {
    filter->runtime_state.handheld.blur_velocity = 0.0f;
  }
  blur_r = clamp_float(blur_r, 0.0f, 20.0f);

  gs_texture_t *tex_to_use = source_tex;

  gs_texrender_reset(output_buf);
  if (!gs_texrender_begin(output_buf, width, height)) {
    EMULENS_LOG(LOG_DEBUG, "handheld: output texrender begin failed");
    return NULL;
  }

  struct vec4 clear_color;
  vec4_set(&clear_color, 0.0f, 0.0f, 0.0f, 0.0f);
  gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);

  bind_common_parameters(filter, tex_to_use, width, height);

  // Re-bind mode-specific settings after the common bind (per-mode shader
  // uniforms can be flushed by the matrix setup on some backends). The
  // per-frame motion offsets above are already bound and unaffected.
  rebind_current_settings(filter);

  SET_PARAM_FLOAT(filter->common.blur_radius, blur_r);
  bool pass_ok = lens_run_technique_quad(filter->tech.handheld, width, height);
  gs_texrender_end(output_buf);

  gs_texture_t *output_tex = gs_texrender_get_texture(output_buf);
  return pass_ok ? output_tex : source_tex;
}

void handheld_destroy_cpu(struct lens_data *filter) {
  if (!filter)
    return;
  vec2_set(&filter->runtime_state.handheld.smoothed_offset, 0.0f, 0.0f);
  vec2_set(&filter->runtime_state.handheld.prev_offset, 0.0f, 0.0f);
  filter->runtime_state.handheld.smoothed_rot_rad = 0.0f;
  filter->runtime_state.handheld.prev_rot_rad = 0.0f;
  filter->runtime_state.handheld.smoothed_zoom_factor = 1.0f;
  filter->runtime_state.handheld.prev_zoom_factor = 1.0f;
  filter->runtime_state.handheld.prev_elapsed_sec = 0.0f;
  filter->runtime_state.handheld.blur_velocity = 0.0f;
  filter->runtime_state.handheld.motion_initialized = false;
}
void handheld_destroy(struct lens_data *filter) { (void)filter; }

void handheld_get_defaults(obs_data_t *settings) {
  // Natural operator motion: small positional drift, gentle correction and
  // inertia, no baked-in motion blur or walking bob.
  obs_data_set_default_double(settings, P_HAND_POS_AMP, HAND_POS_AMP_DEFAULT);
  obs_data_set_default_double(settings, P_HAND_POS_SPEED, HAND_POS_SPEED_DEFAULT);
  obs_data_set_default_double(settings, P_HAND_ROT_AMP, HAND_ROT_AMP_DEFAULT);
  obs_data_set_default_double(settings, P_HAND_ROT_SPEED, 0.12);
  obs_data_set_default_double(settings, P_HAND_ZOOM_AMP, 0.004);
  obs_data_set_default_double(settings, P_HAND_ZOOM_SPEED, 0.10);
  obs_data_set_default_bool(settings, P_HAND_BLUR_ENABLE, false);
  obs_data_set_default_double(settings, P_HAND_BLUR_AMP, 0.25);
  obs_data_set_default_double(settings, P_HAND_BLUR_SPEED, 0.20);
  obs_data_set_default_double(settings, P_HAND_BLUR_STATIC, 0.05);
  obs_data_set_default_double(settings, P_HAND_FEATHER, 0.04);
  obs_data_set_default_double(settings, P_HAND_TREMOR_AMP,
                              HAND_TREMOR_AMP_DEFAULT);
  obs_data_set_default_double(settings, P_HAND_BREATH_AMP,
                              HAND_BREATH_AMP_DEFAULT);
  obs_data_set_default_double(settings, P_HAND_STEP_AMP, HAND_STEP_AMP_DEFAULT);
  obs_data_set_default_double(settings, P_HAND_STEP_SPEED,
                              HAND_STEP_SPEED_DEFAULT);
  obs_data_set_default_double(settings, P_HAND_CORRECTION_AMP,
                              HAND_CORRECTION_AMP_DEFAULT);
  obs_data_set_default_double(settings, P_HAND_INERTIA, HAND_INERTIA_DEFAULT);
  obs_data_set_default_double(settings, P_HAND_BLUR_RESPONSE,
                              HAND_BLUR_RESPONSE_DEFAULT);
  obs_data_set_default_double(settings, P_HAND_VEL_DIR_X, 0.0);
  obs_data_set_default_double(settings, P_HAND_VEL_DIR_Y, 0.0);
  obs_data_set_default_double(settings, P_HAND_VEL_MAG, 0.0);
  obs_data_set_default_bool(settings, P_HAND_DIR_BLUR_ENABLE, false);
  obs_data_set_default_bool(settings, P_HAND_BLUR_DIRECTIONAL, false);
}

void handheld_populate_presets(obs_property_t *list) {
  ADD_PRESET_ENTRY(list, "HandheldDefault", "default_handheld",
                   "HandheldDefaultDesc");
  ADD_PRESET_ENTRY(list, "HandheldRun", "handheld_run", "HandheldRunDesc");
  ADD_PRESET_ENTRY(list, "HandheldBreath", "handheld_breathing",
                   "HandheldBreathDesc");
  ADD_PRESET_ENTRY(list, "HandheldDrift", "handheld_drift",
                   "HandheldDriftDesc");
}
