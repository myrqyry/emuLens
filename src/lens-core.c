#include "lens-data.h"
#include "obs-utils.h"
#include "strategies/strategies.h"
#include <string.h>
// Lifecycle - Handled in plugin-main.c

static inline uint32_t xorshift32(uint32_t *state) {
  uint32_t x = *state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return (*state = x);
}

void lens_draw_quad(uint32_t cx, uint32_t cy) {
  if (cx == 0 || cy == 0)
    return;
  gs_draw_sprite(NULL, 0, cx, cy);
}

// -------------------------------------------------------------------------------------------------
// #6 - GPU Capability Detection
// -------------------------------------------------------------------------------------------------

static void detect_gpu_capabilities(struct lens_data *filter) {
  if (filter->gpu_caps.capabilities_detected)
    return;

  /*
   * Called once from lens_create() while OBS is already inside
   * obs_enter_graphics(). The capabilities_detected guard prevents re-entry
   * from lens_render(). Do not enter/leave graphics here.
   */

  filter->gpu_caps.high_precision_float = true;

  const char *renderer = (const char *)gs_get_device_name();
  int device_type = gs_get_device_type();
  if (renderer) {
    EMULENS_LOG(LOG_INFO, u8"GPU detected:");
    EMULENS_LOG(LOG_INFO, u8"  └─ Device : %s", renderer);
    // The high_precision_float concern is OpenGL-specific (mediump/highp
    // precision qualifiers in GLSL). Discrete GPUs and non-OpenGL backends
    // (D3D11, Metal) handle fp32 uniformly, so we only downgrade on OpenGL.
    if (device_type == GS_DEVICE_OPENGL) {
      // Allowlist of known legacy Intel prefixes. Inverting the logic to
      // an allowlist (rather than denylist of modern cards) keeps us
      // future-proof against new Intel GPU product names (Arc B-series,
      // Battlemage, etc.) without needing periodic denylist updates.
      bool is_legacy_intel =
          strstr(renderer, "Intel") &&
          (strstr(renderer, "HD Graphics") ||
           strstr(renderer, "UHD Graphics") || strstr(renderer, "Iris") ||
           strstr(renderer, "GMA"));
      if (is_legacy_intel) {
        filter->gpu_caps.high_precision_float = false;
        EMULENS_LOG_CONFIG(
            "Reduced precision mode enabled for legacy Intel GPU");
      }
      if (strstr(renderer, "AMD") &&
          (strstr(renderer, "Vega") || strstr(renderer, "Radeon Graphics"))) {
        filter->gpu_caps.high_precision_float = false;
        EMULENS_LOG_CONFIG("Reduced precision mode enabled for older AMD GPU");
      }
    }
  } else {
    EMULENS_LOG_WARN("GPU detection unavailable; using conservative defaults");
  }

  // 8192 matches the conservative fallback used before detection.
  // OBS does not expose a public gs_get_max_texture_size() API;
  // 8192 is safe for all modern D3D11/Metal/Vulkan and most OpenGL targets.
  filter->gpu_caps.max_texture_size = 8192;
  filter->gpu_caps.capabilities_detected = true;
}

/**
 * Applies a two-pass Gaussian blur (H then V) to @tex, writing intermediate
 * results into filter->blur_h_buffer and filter->blur_v_buffer.
 *
 * These buffers are SEPARATE from bloom_render()'s anamorphic buffers
 * (bloom_anam_h / bloom_anam_v). It is safe to call apply_gaussian_blur
 * from inside bloom_render()'s non-anamorphic path.
 *
 * WARNING: Do NOT call apply_gaussian_blur from within any strategy's
 * render callback while another strategy's render is also in-flight on
 * the same filter instance. Strategies are single-active-at-a-time, so
 * this is currently guaranteed by the architecture.
 */
gs_texture_t *apply_gaussian_blur(struct lens_data *filter, gs_texture_t *tex,
                                  uint32_t w, uint32_t h, float radius) {
  if (radius < MIN_BLUR_RADIUS) {
    return tex;
  }
  radius = clamp_float(radius, MIN_BLUR_RADIUS, MAX_BLUR_RADIUS);

  if (!filter->blur_h_buffer || !filter->blur_v_buffer) {
    return tex;
  }

  // H Pass — use bind_common_parameters for consistency with the V pass
  // so elapsed_time, noise_tex, and uv_pixel_interval are seeded the same way.
  gs_texrender_reset(filter->blur_h_buffer);
  if (!gs_texrender_begin(filter->blur_h_buffer, w, h)) {
    return tex;
  }

  bind_common_parameters(filter, tex, w, h);
  if (filter->common.blur_radius)
    SET_PARAM_FLOAT(filter->common.blur_radius, radius);

  gs_technique_t *tech = filter->tech.blur_h;
  if (!tech) {
    gs_texrender_end(filter->blur_h_buffer);
    return tex;
  }
  bool pass_ok = lens_run_technique_quad(tech, w, h);
  gs_texrender_end(filter->blur_h_buffer);
  if (!pass_ok)
    return tex;

  gs_texture_t *h_result = gs_texrender_get_texture(filter->blur_h_buffer);
  if (!h_result)
    return tex;

  // V Pass — use bind_common_parameters for consistent param binding
  gs_texrender_reset(filter->blur_v_buffer);
#ifdef EMULENS_VERBOSE_DEBUG
  EMULENS_LOG(LOG_DEBUG, "begin %s rt=%p tex=%p %ux%u", "blur_v",
              (void *)filter->blur_v_buffer,
              (void *)gs_texrender_get_texture(filter->blur_v_buffer), w, h);
#endif
  if (!gs_texrender_begin(filter->blur_v_buffer, w, h)) {
    return tex;
  }

  bind_common_parameters(filter, h_result, w, h);
  if (filter->common.blur_radius)
    SET_PARAM_FLOAT(filter->common.blur_radius, radius);

  tech = filter->tech.blur_v;
  if (!tech) {
    gs_texrender_end(filter->blur_v_buffer);
    return tex;
  }
  pass_ok = lens_run_technique_quad(tech, w, h);
  gs_texrender_end(filter->blur_v_buffer);
  if (!pass_ok)
    return tex;

  gs_texture_t *final_result = gs_texrender_get_texture(filter->blur_v_buffer);
  return final_result ? final_result : tex;
}

static void validate_param_availability(struct lens_data *filter) {
  if (filter->param_validation.validation_complete)
    return;

  struct {
    const char *name;
    gs_eparam_t *param;
    bool required;
  } param_check[] = {
      {"elapsed_time", filter->common.elapsed_time, true},
      {"uv_pixel_interval", filter->common.uv_pixel_interval, true},
      {"image", filter->common.main_tex, true},
      {"ViewProj", filter->common.ViewProj, false},
      {"val_blur_radius", filter->common.blur_radius, false},
  };

  bool critical_params_ok = true;
  size_t num_checks = sizeof(param_check) / sizeof(param_check[0]);
  for (size_t i = 0; i < num_checks; i++) {
    if (!param_check[i].param) {
      if (param_check[i].required) {
        EMULENS_LOG_ERROR("CRITICAL: Required parameter '%s' not found",
                          param_check[i].name);
        critical_params_ok = false;
      } else {
        EMULENS_LOG(LOG_WARNING,
                    "Optional parameter '%s' not found — dependent effects may "
                    "render incorrectly",
                    param_check[i].name);
      }
    }
  }

  filter->param_validation.critical_params_valid = critical_params_ok;
  filter->param_validation.validation_complete = true;
}

/*
 * Dimension-preserving filters must query the base size of their immediate
 * filter target.  Calling obs_source_get_width()/height() on a non-filter
 * parent walks that parent's filter chain and can recurse back into EmuLens.
 * OBS's built-in Scroll filter uses this same base-dimension pattern.
 */
static bool lens_get_target_base_size(struct lens_data *filter,
                                      uint32_t *out_width,
                                      uint32_t *out_height) {
  if (!filter || !filter->context || !out_width || !out_height)
    return false;

  obs_source_t *target = obs_filter_get_target(filter->context);
  if (!target)
    return false;

  uint32_t width = obs_source_get_base_width(target);
  uint32_t height = obs_source_get_base_height(target);
  if (width == 0 || height == 0) {
    int misses = emulens_atomic_load(&filter->source_miss_count);
    if (misses < 3) {
      obs_source_t *parent = obs_filter_get_parent(filter->context);
      EMULENS_LOG_WARN(
          "Dimension probe: target='%s' type=%d base=%ux%u parent='%s' "
          "parent_base=%ux%u",
          obs_source_get_name(target), (int)obs_source_get_type(target), width,
          height, parent ? obs_source_get_name(parent) : "(null)",
          parent ? obs_source_get_base_width(parent) : 0,
          parent ? obs_source_get_base_height(parent) : 0);
      emulens_atomic_store(&filter->source_miss_count, misses + 1);
    }
    return false;
  }

  *out_width = width;
  *out_height = height;
  emulens_atomic_store(&filter->last_source_width, (int)width);
  emulens_atomic_store(&filter->last_source_height, (int)height);
  emulens_atomic_store(&filter->source_miss_count, 0);
  return true;
}

static bool lens_get_render_size(struct lens_data *filter, uint32_t *out_width,
                                 uint32_t *out_height) {
  if (lens_get_target_base_size(filter, out_width, out_height))
    return true;

  int lw = emulens_atomic_load(&filter->last_source_width);
  int lh = emulens_atomic_load(&filter->last_source_height);
  if (lw > 0 && lh > 0) {
    int miss = emulens_atomic_load(&filter->source_miss_count) + 1;
    emulens_atomic_store(&filter->source_miss_count, miss);
    if (miss <= 120) {
      *out_width = (uint32_t)lw;
      *out_height = (uint32_t)lh;
      return true;
    }
    emulens_atomic_store(&filter->last_source_width, 0);
    emulens_atomic_store(&filter->last_source_height, 0);
  }

  *out_width = 0;
  *out_height = 0;
  return false;
}

static const char *lens_get_name(void *unused) {
  UNUSED_PARAMETER(unused);
  return "EmuLens";
}

static void clear_all_shared_texrenders(struct lens_data *filter);

/*
 * Early Vidicon revisions wrote a very different set of explicit values into
 * scene collections.  In particular, a luma threshold of 1.0 makes the
 * recovered persistence model impossible to excite on ordinary SDR footage.
 *
 * Do not reset every old/custom Vidicon configuration.  Only migrate the
 * unmistakable churn-era state: the newer per-channel lag keys are absent and
 * several of the old artifact controls are simultaneously at extreme values.
 */
static bool vidicon_needs_legacy_migration(obs_data_t *settings) {
  if (!settings)
    return false;

  if (obs_data_has_user_value(settings, P_VID_RED_LAG) ||
      obs_data_has_user_value(settings, P_VID_BLUE_LAG) ||
      obs_data_has_user_value(settings, P_VID_GAMMA))
    return false;

  int legacy_score = 0;
  if (obs_data_get_double(settings, P_VID_THRESHOLD) >= 0.995)
    legacy_score += 2;
  if (obs_data_get_double(settings, P_VID_BLEND) >= 0.95 &&
      !obs_data_get_bool(settings, P_VID_SOURCE_COLOR))
    legacy_score++;
  if (obs_data_get_double(settings, P_VID_REGISTER) >= 1.0)
    legacy_score++;
  if (obs_data_get_double(settings, P_VID_REGISTER_DRIFT) >= 1.0)
    legacy_score++;
  if (obs_data_get_double(settings, P_VID_SCANLINES) >= 0.25)
    legacy_score++;
  if (obs_data_get_double(settings, P_VID_HALATION) >= 0.25)
    legacy_score++;
  if (obs_data_get_double(settings, P_VID_HUM) >= 0.20)
    legacy_score++;

  return legacy_score >= 4;
}

static void migrate_legacy_vidicon_settings(obs_data_t *settings) {
  if (!vidicon_needs_legacy_migration(settings))
    return;

  EMULENS_LOG(LOG_INFO,
              "Migrating legacy Vidicon settings to recovered tube defaults");

  obs_data_set_double(settings, P_VID_DECAY, 0.80);
  obs_data_set_double(settings, P_VID_THRESHOLD, 0.64);
  obs_data_set_int(settings, P_VID_COLOR, 0xFFFFFFFF);
  obs_data_set_double(settings, P_VID_BLEND, 0.0);
  obs_data_set_double(settings, P_VID_RED_LAG, 0.28);
  obs_data_set_double(settings, P_VID_BLUE_LAG, 0.10);
  obs_data_set_bool(settings, P_VID_COMET, false);
  obs_data_set_double(settings, P_VID_COMET_LENGTH, 4.0);
  obs_data_set_double(settings, P_VID_NOISE, 0.012);
  obs_data_set_double(settings, P_VID_BEAM_PULL, 0.0);
  obs_data_set_double(settings, P_VID_LAG_NONLINEAR, 0.12);
  obs_data_set_double(settings, P_VID_REGISTER, 0.02);
  obs_data_set_double(settings, P_VID_REGISTER_DRIFT, 0.01);
  obs_data_set_double(settings, P_VID_SCANLINES, 0.008);
  obs_data_set_double(settings, P_VID_GAMMA, 1.0);
  obs_data_set_double(settings, P_VID_HALATION, 0.035);
  obs_data_set_double(settings, P_VID_HUM, 0.006);
  obs_data_set_bool(settings, P_VID_SOURCE_COLOR, true);
  obs_data_set_bool(settings, P_VID_SMEAR_ENABLE, false);
  obs_data_set_int(settings, P_VID_SMEAR_MODE, 0);
  obs_data_set_int(settings, P_VID_SMEAR_DIRECTION, 0);
  obs_data_set_double(settings, P_VID_SMEAR_STRENGTH, 0.0);
}

static void lens_update(void *data, obs_data_t *settings) {
  if (!data || !settings)
    return;
  struct lens_data *filter = data;

  // Bail if destroy has started — filter resources may vanish mid-flight.
  if (atomic_load_explicit(&filter->destroying, memory_order_acquire))
    return;

  int requested_mode = (int)obs_data_get_int(settings, UI_MODE);
  if (requested_mode == MODE_VIDICON)
    migrate_legacy_vidicon_settings(settings);

  obs_data_t *settings_copy = obs_data_create();
  if (!settings_copy) {
    EMULENS_LOG_WARN("Failed to snapshot filter settings");
    return;
  }
  emulens_get_defaults(settings_copy);
  obs_data_apply(settings_copy, settings);

  int new_mode = (int)obs_data_get_int(settings_copy, UI_MODE);
  if (new_mode < 0 || new_mode >= NUM_MODES)
    new_mode = MODE_VIDICON;

  int old_mode = (int)atomic_load_explicit(&filter->mode, memory_order_acquire);

  // Ownership transfer pattern: settings_copy is created with refcount 1.
  // We hand it off to cached_settings (no addref) and never release it at
  // the end of this function — that ownership transfer is the single point
  // where the refcount drops from 1 to 0 when later replaced.
  pthread_mutex_lock(&filter->settings_mutex);
  if (filter->cached_settings)
    obs_data_release(filter->cached_settings);
  filter->cached_settings = settings_copy;
  pthread_mutex_unlock(&filter->settings_mutex);

  if (atomic_load_explicit(&filter->params_cached, memory_order_acquire) &&
      old_mode >= 0 && old_mode < NUM_MODES) {
    // Enter graphics BEFORE render_mutex to avoid AB-BA deadlock:
    //   render thread: graphics (implicit) → render_mutex
    //   update thread: render_mutex → graphics  ← DEADLOCK
    // Fixed ordering: graphics → render_mutex (both threads consistent)
    obs_enter_graphics();
    pthread_mutex_lock(&filter->render_mutex);
    if (old_mode != new_mode) {
      if (strategies[old_mode].destroy)
        strategies[old_mode].destroy(filter);
      if (strategies[old_mode].destroy_cpu)
        strategies[old_mode].destroy_cpu(filter);
      memset(&filter->mode_params, 0, sizeof(filter->mode_params));
      emulens_atomic_store(&filter->cur_write, 0);
    }
    atomic_store_explicit(&filter->mode, new_mode, memory_order_release);
    // Cache params for the new mode before updating settings — ensures
    // shader handles are populated even if this is the first activation
    // after a mode switch (cache_params for all modes runs once at
    // create-time, but stale/NULL handles can occur after shader
    // recompilation on some backends). Compile-time dispatch via
    // EMULENS_MODES X-macro: the active mode's cache_params call is inlined
     // as a switch case so the compiler can devirtualize the vtable lookup.
     // The vtable remains the
    // single source of truth (see strategies.c); this loop is the
    // activation-order policy, not a parallel implementation.
    //
    // The strategy receives its own addref'd reference; released below.
    // Cache params for the new mode (GET-only lookups — safe inside graphics).
    obs_data_addref(settings_copy);
#define X(id, suffix, display_name, tech)                                      \
  case MODE_##id: {                                                            \
    const lens_effect_vtable_t *active = &strategies[MODE_##id];               \
    if (active->cache_params)                                                  \
      active->cache_params(filter, filter->effect);                            \
    break;                                                                     \
  }
    switch (new_mode) { EMULENS_MODES }
#undef X
    obs_data_release(settings_copy);
    clear_all_shared_texrenders(filter);
    pthread_mutex_unlock(&filter->render_mutex);
    obs_leave_graphics();

    // lens_render updates settings every frame after leaving this explicit
    // graphics context. SET_PARAM_* calls must not run inside
    // obs_enter_graphics().
  }
}

typedef enum {
  TEXRENDER_NO_CHANGE = 0,
  TEXRENDER_RESIZED = 1,
  TEXRENDER_FAILED = 2,
} texrender_result_t;

static inline void safe_texrender_destroy(gs_texrender_t **rt) {
  if (*rt) {
    gs_texrender_destroy(*rt);
    *rt = NULL;
  }
}

static bool bloom_pyramid_buffers_ready(const struct lens_data *filter) {
  for (size_t i = 0; i < BLOOM_PYRAMID_LEVELS; i++) {
    if (!filter->bloom_pyramid[i])
      return false;
  }
  for (size_t i = 0; i + 1 < BLOOM_PYRAMID_LEVELS; i++) {
    if (!filter->bloom_upsample[i])
      return false;
  }
  return true;
}

static void destroy_bloom_pyramid_buffers(struct lens_data *filter) {
  for (size_t i = 0; i < BLOOM_PYRAMID_LEVELS; i++)
    safe_texrender_destroy(&filter->bloom_pyramid[i]);
  for (size_t i = 0; i + 1 < BLOOM_PYRAMID_LEVELS; i++)
    safe_texrender_destroy(&filter->bloom_upsample[i]);
}

static bool create_bloom_pyramid_buffers(struct lens_data *filter) {
  destroy_bloom_pyramid_buffers(filter);
  for (size_t i = 0; i < BLOOM_PYRAMID_LEVELS; i++)
    filter->bloom_pyramid[i] = gs_texrender_create(GS_RGBA16F, GS_ZS_NONE);
  for (size_t i = 0; i + 1 < BLOOM_PYRAMID_LEVELS; i++)
    filter->bloom_upsample[i] = gs_texrender_create(GS_RGBA16F, GS_ZS_NONE);
  return bloom_pyramid_buffers_ready(filter);
}

static void clear_texrender_if_valid(gs_texrender_t *texrender, uint32_t width,
                                     uint32_t height) {
  if (!texrender)
    return;
  gs_texrender_reset(texrender);
  if (gs_texrender_begin(texrender, width, height)) {
    struct vec4 black = {0};
    gs_clear(GS_CLEAR_COLOR, &black, 0.0f, 0);
    gs_texrender_end(texrender);
  } else {
    EMULENS_LOG(LOG_DEBUG,
                "clear_texrender_if_valid: gs_texrender_begin failed");
  }
}

static void clear_all_shared_texrenders(struct lens_data *filter) {
  uint32_t w = filter->allocated_width ? filter->allocated_width : 1;
  uint32_t h = filter->allocated_height ? filter->allocated_height : 1;
  gs_texrender_t *targets[] = {
      filter->texrenders[0],      filter->texrenders[1],
      filter->vidicon_history[0], filter->vidicon_history[1],
      filter->output_texrender,   filter->blur_h_buffer,
      filter->blur_v_buffer,      filter->bloom_anam_h,
      filter->bloom_anam_v,       filter->input_texrender,
  };

  for (size_t i = 0; i < sizeof(targets) / sizeof(targets[0]); i++)
    clear_texrender_if_valid(targets[i], w, h);
}

static texrender_result_t
recreate_texrenders_if_needed(struct lens_data *filter, uint32_t width,
                              uint32_t height) {
  if (width == 0 || height == 0) {
    EMULENS_LOG_WARN("Invalid texrender dimensions: %ux%u", width, height);
    return TEXRENDER_FAILED;
  }

  uint32_t max_safe_dimension =
      filter->gpu_caps.capabilities_detected
          ? (uint32_t)filter->gpu_caps.max_texture_size
          : 8192u;
  if (width > max_safe_dimension || height > max_safe_dimension) {
    EMULENS_LOG_ERROR(
        "Dimension %ux%u exceeds safe GPU limit %u — cannot allocate", width,
        height, max_safe_dimension);
    return TEXRENDER_FAILED;
  }

  if (filter->texrenders[0] && filter->texrenders[1] &&
      filter->vidicon_history[0] && filter->vidicon_history[1] &&
      filter->blur_h_buffer && filter->blur_v_buffer &&
      filter->input_texrender && filter->output_texrender &&
      filter->bloom_anam_h && filter->bloom_anam_v &&
      bloom_pyramid_buffers_ready(filter) &&
      filter->allocated_width == width && filter->allocated_height == height) {
    filter->consecutive_thrash = 0;
    return TEXRENDER_NO_CHANGE;
  }

  uint64_t now = os_gettime_ns();
  if (now - filter->last_realloc_time < 500000000ULL) {
    filter->consecutive_thrash++;
    if (filter->consecutive_thrash > 8) {
      EMULENS_LOG_ERROR("Texture thrashing detected (>8 reallocs in 500ms)! "
                        "Aborting render");
      return TEXRENDER_FAILED;
    }
  } else {
    filter->consecutive_thrash = 0;
  }
  filter->last_realloc_time = now;

  EMULENS_LOG(LOG_DEBUG, "Allocating GPU buffers for %ux%u", width, height);

#define RECREATE_RENDER(r)                                                     \
  do {                                                                         \
    if (r) {                                                                   \
      gs_texrender_destroy(r);                                                 \
      r = NULL;                                                                \
    }                                                                          \
    r = gs_texrender_create(GS_RGBA, GS_ZS_NONE);                              \
  } while (0)

  RECREATE_RENDER(filter->texrenders[0]);
  RECREATE_RENDER(filter->texrenders[1]);
  RECREATE_RENDER(filter->vidicon_history[0]);
  RECREATE_RENDER(filter->vidicon_history[1]);
  RECREATE_RENDER(filter->blur_h_buffer);
  RECREATE_RENDER(filter->blur_v_buffer);
  RECREATE_RENDER(filter->input_texrender);
  RECREATE_RENDER(filter->output_texrender);
  RECREATE_RENDER(filter->bloom_anam_h);
  RECREATE_RENDER(filter->bloom_anam_v);
  if (!create_bloom_pyramid_buffers(filter)) {
    EMULENS_LOG_ERROR("Failed to allocate bloom pyramid buffers");
  }

#undef RECREATE_RENDER

  if (!filter->texrenders[0] || !filter->texrenders[1] ||
      !filter->vidicon_history[0] || !filter->vidicon_history[1] ||
      !filter->blur_h_buffer || !filter->blur_v_buffer ||
      !filter->input_texrender || !filter->output_texrender ||
      !filter->bloom_anam_h || !filter->bloom_anam_v ||
      !bloom_pyramid_buffers_ready(filter))
    return TEXRENDER_FAILED;

  filter->allocated_width = width;
  filter->allocated_height = height;
  clear_all_shared_texrenders(filter);
  filter->consecutive_thrash = 0;
  return TEXRENDER_RESIZED;
}

static void lens_tick(void *data, float seconds) {
  if (!data)
    return;
  struct lens_data *filter = data;
  if (atomic_load_explicit(&filter->destroying, memory_order_acquire))
    return;

  int64_t add_ns = (int64_t)((double)seconds * 1000000000.0);
  int64_t current = emulens_atomic_load(&filter->elapsed_time_ns);
  int64_t wrap_ns = (int64_t)(TIME_WRAP_THRESHOLD * 1000000000.0);
  int64_t new_val = current + add_ns;
  if (new_val > wrap_ns) {
    new_val -= wrap_ns;
  }
  emulens_atomic_store(&filter->elapsed_time_ns, new_val);
}

#define GET_TECH_SAFE(var, effect, name)                                       \
  do {                                                                         \
    var = gs_effect_get_technique(effect, name);                               \
    if (!var) {                                                                \
      EMULENS_LOG_ERROR("Shader technique '%s' not found!", name);             \
    } else {                                                                   \
      EMULENS_LOG(LOG_DEBUG, "Cached technique '%s' at %p", name,              \
                  (void *)var);                                                \
    }                                                                          \
  } while (0)

static void recache_params_after_compile(struct lens_data *filter) {
  /* Re-lookup all parameters after at least one technique has been
   * compiled.  OBS's Cg backend resolves parameter types during
   * technique compilation, so handles cached earlier may have
   * stale/unknown types.  Reset validation so a follow-up
   * validate_param_availability() call re-evaluates from scratch. */
  filter->param_validation.validation_complete = false;
  filter->param_validation.critical_params_valid = false;
  GET_PARAM_SAFE(filter->common.elapsed_time, filter->effect, "elapsed_time");
  GET_PARAM_SAFE(filter->common.uv_pixel_interval, filter->effect,
                 "uv_pixel_interval");
  GET_PARAM_SAFE(filter->common.ViewProj, filter->effect, "ViewProj");
  GET_PARAM_SAFE(filter->common.main_tex, filter->effect, "image");
  GET_PARAM_SAFE(filter->common.blur_radius, filter->effect, "val_blur_radius");
  GET_PARAM_SAFE(filter->common.blurred_image, filter->effect, "blurred_image");
  GET_PARAM_SAFE(filter->common.noise_tex, filter->effect, "noise_tex");

  int active_mode = (int)emulens_atomic_load(&filter->mode);
  if (active_mode >= 0 && active_mode < NUM_MODES) {
    if (strategies[active_mode].cache_params) {
      EMULENS_LOG(LOG_DEBUG, "Recaching parameters for active strategy index %d", active_mode);
      strategies[active_mode].cache_params(filter, filter->effect);
    }
  }
}

static void *lens_create(obs_data_t *settings, obs_source_t *context) {
  struct lens_data *filter = bzalloc(sizeof(struct lens_data));
  if (!filter) {
    EMULENS_LOG_ERROR("lens_create: bzalloc returned NULL");
    return NULL;
  }

  pthread_mutex_init(&filter->settings_mutex, NULL);

  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&filter->render_mutex, &attr);
  pthread_mutexattr_destroy(&attr);

  filter->context = context;
  atomic_store_explicit(&filter->mode, MODE_VIDICON, memory_order_release);
  atomic_store_explicit(&filter->destroying, false, memory_order_release);
  atomic_store_explicit(&filter->params_cached, false, memory_order_release);

  // Generate procedural noise texture for shader optimization.
  uint8_t *noise_data = bzalloc(NOISE_TEX_SIZE * NOISE_TEX_SIZE);
  if (!noise_data) {
    EMULENS_LOG_ERROR("Failed to allocate noise texture buffer (%d bytes)",
                      NOISE_TEX_SIZE * NOISE_TEX_SIZE);
    pthread_mutex_destroy(&filter->settings_mutex);
    pthread_mutex_destroy(&filter->render_mutex);
    bfree(filter);
    return NULL;
  }

  uint32_t rng = 0xDEADBEEF ^ (uint32_t)(uintptr_t)filter;
  for (int i = 0; i < NOISE_TEX_SIZE * NOISE_TEX_SIZE; ++i) {
    noise_data[i] = (uint8_t)(xorshift32(&rng) & 0xFF);
  }
  const uint8_t *data_ptr = noise_data;

  char *effect_path = obs_module_file("emulens.effect");
  if (!effect_path) {
    EMULENS_LOG_CRITICAL_BLOCK(
        "obs_module_file('emulens.effect') returned NULL",
        "Please reinstall the plugin — data directory or effect file is "
        "missing");
    pthread_mutex_destroy(&filter->settings_mutex);
    pthread_mutex_destroy(&filter->render_mutex);
    bfree(noise_data);
    bfree(filter);
    return NULL;
  }

  obs_enter_graphics();

  filter->noise_tex =
      gs_texture_create(NOISE_TEX_SIZE, NOISE_TEX_SIZE, GS_R8, 1, &data_ptr, 0);
  bfree(noise_data);
  if (!filter->noise_tex) {
    EMULENS_LOG_WARN(
        "Failed to create noise texture (256×256 R8); "
        "grain/grit effects may appear uniform — using 1×1 fallback");
    // Create 1x1 white fallback
    uint8_t white = 0xFF;
    const uint8_t *wp = &white;
    filter->noise_tex = gs_texture_create(1, 1, GS_R8, 1, &wp, 0);
  }

  filter->consecutive_thrash = 0;
  filter->last_realloc_time = 0;
  filter->elapsed_time_ns = 0;

  filter->runtime_state.handheld.phase_seed =
      (float)(xorshift32(&rng) & 0xFFFF) / 65536.0f * 1000.0f;
  filter->runtime_state.handheld.smoothed_zoom_factor = 1.0f;
  filter->runtime_state.handheld.prev_zoom_factor = 1.0f;

  // GPU caps detection deferred to first lens_render() where a valid graphics
  // context is guaranteed on all platforms.
  char *shader_string = load_shader_from_file(effect_path);
  char *error_string = NULL;
  if (shader_string) {
    filter->effect = gs_effect_create(shader_string, effect_path, &error_string);
    bfree(shader_string);
  } else {
    filter->effect = gs_effect_create_from_file(effect_path, &error_string);
  }
  bfree(effect_path);

  if (!filter->effect) {
    EMULENS_LOG_CRITICAL_BLOCK(
        error_string ? error_string : "gs_effect_create_from_file failed",
        "Please reinstall the plugin — shader file may be corrupt");
    bfree(error_string);
    if (filter->noise_tex)
      gs_texture_destroy(filter->noise_tex);
    obs_leave_graphics();
    pthread_mutex_destroy(&filter->settings_mutex);
    pthread_mutex_destroy(&filter->render_mutex);
    bfree(filter);
    return NULL;
  }
  bfree(error_string);

  // Cache techniques
  GET_TECH_SAFE(filter->tech.draw, filter->effect, "Draw");
  GET_TECH_SAFE(filter->tech.blur_h, filter->effect, "DrawGaussianH");
  GET_TECH_SAFE(filter->tech.blur_v, filter->effect, "DrawGaussianV");
  GET_TECH_SAFE(filter->tech.vidicon, filter->effect, "DrawVidicon");
  GET_TECH_SAFE(filter->tech.vidicon_composite, filter->effect,
                "DrawVidiconComposite");
  GET_TECH_SAFE(filter->tech.starburst, filter->effect, "DrawStarburst");
  GET_TECH_SAFE(filter->tech.leak, filter->effect, "DrawLightLeak");
  GET_TECH_SAFE(filter->tech.handheld, filter->effect, "DrawHandheld");
  GET_TECH_SAFE(filter->tech.bokeh, filter->effect, "DrawBokeh");
  GET_TECH_SAFE(filter->tech.particles, filter->effect, "DrawBokehParticles");
  GET_TECH_SAFE(filter->tech.orton, filter->effect, "DrawOrton");
  GET_TECH_SAFE(filter->tech.bloom_extract, filter->effect, "DrawBloomExtract");
  GET_TECH_SAFE(filter->tech.bloom_downsample, filter->effect,
                "DrawBloomDownsample");
  GET_TECH_SAFE(filter->tech.bloom_upsample, filter->effect,
                "DrawBloomUpsample");
  GET_TECH_SAFE(filter->tech.bloom_combine, filter->effect, "DrawBloomCombine");
  GET_TECH_SAFE(filter->tech.grit, filter->effect, "DrawGrit");
  GET_TECH_SAFE(filter->tech.mirage, filter->effect, "DrawMirage");
  GET_TECH_SAFE(filter->tech.spot, filter->effect, "DrawCenterSpot");
  GET_TECH_SAFE(filter->tech.pinhole, filter->effect, "DrawPinhole");
  GET_TECH_SAFE(filter->tech.polaroid, filter->effect, "DrawPolaroid");
  GET_TECH_SAFE(filter->tech.daguerreotype, filter->effect,
                "DrawDaguerreotype");
  GET_TECH_SAFE(filter->tech.technicolor, filter->effect, "DrawTechnicolor");
  GET_TECH_SAFE(filter->tech.wet_lens, filter->effect, "DrawWetLens");
  GET_TECH_SAFE(filter->tech.phone_wet, filter->effect, "DrawPhoneWet");

  // Validate that every strategy's tech_name resolved to a real technique.
  for (int i = 0; i < NUM_MODES; i++) {
    if (!strategies[i].tech_name) {
      EMULENS_LOG_WARN("Strategy[%d] has NULL tech_name — corrupt vtable", i);
      continue;
    }
    gs_technique_t *t =
        gs_effect_get_technique(filter->effect, strategies[i].tech_name);
    if (!t) {
      EMULENS_LOG_ERROR("Strategy[%d] tech_name '%s' not found in shader — "
                        "render will produce blank output",
                        i, strategies[i].tech_name);
    }
  }

  // Bind sampler uniforms needed for warmup texture binding. These are
  // re-cached after compilation via recache_params_after_compile().
  filter->common.main_tex =
      gs_effect_get_param_by_name(filter->effect, "image");
  filter->common.blurred_image =
      gs_effect_get_param_by_name(filter->effect, "blurred_image");
  filter->common.noise_tex =
      gs_effect_get_param_by_name(filter->effect, "noise_tex");
  filter->common.elapsed_time =
      gs_effect_get_param_by_name(filter->effect, "elapsed_time");
  filter->common.uv_pixel_interval =
      gs_effect_get_param_by_name(filter->effect, "uv_pixel_interval");
  filter->common.ViewProj =
      gs_effect_get_param_by_name(filter->effect, "ViewProj");
  filter->common.blur_radius =
      gs_effect_get_param_by_name(filter->effect, "val_blur_radius");

  // Warm up all techniques to force Cg shader compilation so parameter
  // types are resolved before any render or update_settings calls.
  // A valid render target AND valid textures bound to all sampler uniforms
  // are required for Cg compilation to succeed.
  //
  // IMPORTANT: We must call gs_effect_set_* directly (not via SET_PARAM_*)
  // because SET_PARAM_DIAGNOSTIC checks param->info.type, which is still
  // UNKNOWN before compilation -- creating a chicken-and-egg problem.
  gs_texrender_t *warmup_rt = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
  if (warmup_rt) {
    // Create a 1x1 white texture as warmup input
    uint8_t white_pixel[4] = {255, 255, 255, 255};
    const uint8_t *warmup_data = white_pixel;
    gs_texture_t *warmup_tex =
        gs_texture_create(1, 1, GS_RGBA, 1, &warmup_data, 0);

    if (gs_texrender_begin(warmup_rt, 2, 2)) {
      gs_clear(GS_CLEAR_COLOR, &(struct vec4){{0, 0, 0, 0}}, 0.0f, 0);
      gs_ortho(0.0f, 2.0f, 2.0f, 0.0f, -1.0f, 1.0f);
      gs_matrix_identity();

      // Bind a 1x1 white texture to force Cg compilation. Skip params whose
      // type is still UNKNOWN — the effect hasn't compiled yet and OBS
      // rejects the call ("effect_setval_inline: invalid param").
      if (warmup_tex) {
        struct gs_effect_param_info info;
#define WARMUP_BIND_TEX(p)                                                     \
  do {                                                                         \
    if (p) {                                                                   \
      gs_effect_get_param_info(p, &info);                                      \
      if (info.type != GS_SHADER_PARAM_UNKNOWN)                                \
        gs_effect_set_texture(p, warmup_tex);                                   \
    }                                                                          \
  } while (0)
        WARMUP_BIND_TEX(filter->common.main_tex);
        WARMUP_BIND_TEX(filter->common.blurred_image);
        WARMUP_BIND_TEX(filter->common.noise_tex);
        gs_eparam_t *warmup_history =
            gs_effect_get_param_by_name(filter->effect, "history_image");
        WARMUP_BIND_TEX(warmup_history);
#undef WARMUP_BIND_TEX
      }

      gs_technique_t *warmup_techs[] = {
          filter->tech.draw,
          filter->tech.blur_h,
          filter->tech.blur_v,
          filter->tech.vidicon_composite,
          filter->tech.bloom_downsample,
          filter->tech.bloom_upsample,
      };
      for (size_t i = 0; i < sizeof(warmup_techs) / sizeof(warmup_techs[0]);
           i++) {
        gs_technique_t *tech = warmup_techs[i];
        if (!tech)
          continue;
        size_t passes = gs_technique_begin(tech);
        if (passes > 0) {
          for (size_t p = 0; p < passes; p++) {
            if (!gs_technique_begin_pass(tech, (uint32_t)p))
              break;
            lens_draw_quad(2, 2);
            gs_technique_end_pass(tech);
          }
          gs_technique_end(tech);
        }
      }
      // Warm up per-strategy techniques from the vtable — automatically
      // covers any new mode added to EMULENS_MODES.
      for (int i = 0; i < NUM_MODES; i++) {
        if (!strategies[i].tech_name)
          continue;
        gs_technique_t *tech =
            gs_effect_get_technique(filter->effect, strategies[i].tech_name);
        if (!tech)
          continue;
        size_t passes = gs_technique_begin(tech);
        if (passes > 0) {
          for (size_t p = 0; p < passes; p++) {
            if (!gs_technique_begin_pass(tech, (uint32_t)p))
              break;
            lens_draw_quad(2, 2);
            gs_technique_end_pass(tech);
          }
          gs_technique_end(tech);
        }
      }
      gs_texrender_end(warmup_rt);
    }
    if (warmup_tex)
      gs_texture_destroy(warmup_tex);
    gs_texrender_destroy(warmup_rt);
  }

  // Re-cache parameters now that all techniques have been compiled.
  recache_params_after_compile(filter);
  atomic_store_explicit(&filter->params_cached, true, memory_order_release);

  validate_param_availability(filter);

  // Allocation
  filter->texrenders[0] = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
  filter->texrenders[1] = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
  filter->vidicon_history[0] = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
  filter->vidicon_history[1] = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
  filter->blur_h_buffer = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
  filter->blur_v_buffer = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
  filter->input_texrender = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
  filter->output_texrender = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
  filter->bloom_anam_h = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
  filter->bloom_anam_v = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
  bool bloom_pyramid_ok = create_bloom_pyramid_buffers(filter);

  if (!filter->texrenders[0] || !filter->texrenders[1] ||
      !filter->vidicon_history[0] || !filter->vidicon_history[1] ||
      !filter->blur_h_buffer || !filter->blur_v_buffer ||
      !filter->input_texrender || !filter->output_texrender ||
      !filter->bloom_anam_h || !filter->bloom_anam_v || !bloom_pyramid_ok) {
    EMULENS_LOG(LOG_ERROR, u8"CREATE_FAIL: texrender allocation failed");
    EMULENS_LOG(LOG_ERROR, u8"  ├─ texrenders[0] : %p",
                (void *)filter->texrenders[0]);
    EMULENS_LOG(LOG_ERROR, u8"  ├─ texrenders[1] : %p",
                (void *)filter->texrenders[1]);
    EMULENS_LOG(LOG_ERROR, u8"  ├─ blur_h       : %p",
                (void *)filter->blur_h_buffer);
    EMULENS_LOG(LOG_ERROR, u8"  ├─ blur_v       : %p",
                (void *)filter->blur_v_buffer);
    EMULENS_LOG(LOG_ERROR, u8"  ├─ input_rt     : %p",
                (void *)filter->input_texrender);
    EMULENS_LOG(LOG_ERROR, u8"  ├─ output_rt    : %p",
                (void *)filter->output_texrender);
    EMULENS_LOG(LOG_ERROR, u8"  ├─ bloom_anam_h : %p",
                (void *)filter->bloom_anam_h);
    EMULENS_LOG(LOG_ERROR, u8"  └─ bloom_anam_v : %p",
                (void *)filter->bloom_anam_v);

    safe_texrender_destroy(&filter->texrenders[0]);
    safe_texrender_destroy(&filter->texrenders[1]);
    safe_texrender_destroy(&filter->vidicon_history[0]);
    safe_texrender_destroy(&filter->vidicon_history[1]);
    safe_texrender_destroy(&filter->blur_h_buffer);
    safe_texrender_destroy(&filter->blur_v_buffer);
    safe_texrender_destroy(&filter->input_texrender);
    safe_texrender_destroy(&filter->output_texrender);
    safe_texrender_destroy(&filter->bloom_anam_h);
    safe_texrender_destroy(&filter->bloom_anam_v);
    destroy_bloom_pyramid_buffers(filter);
    if (filter->effect)
      gs_effect_destroy(filter->effect);
    if (filter->noise_tex)
      gs_texture_destroy(filter->noise_tex);

    obs_leave_graphics();
    pthread_mutex_destroy(&filter->settings_mutex);
    pthread_mutex_destroy(&filter->render_mutex);
    bfree(filter);
    return NULL;
  }

  // Clear all texrenders to black so first-frame feedback modes
  // (e.g. vidicon) blend against black rather than uninitialized data.
  // clear_all_shared_texrenders uses allocated_width/height (zero at this
  // point) with a 1-pixel fallback to TEXRENDER_CLEAR_SIZE.
  clear_all_shared_texrenders(filter);

  // IMPORTANT: obs_leave_graphics() MUST precede lens_update(). The render
  // path updates shader params after leaving this explicit graphics context;
  // SET_PARAM_* calls must not be nested inside obs_enter_graphics().
  obs_leave_graphics();

  // IMPORTANT: params_cached must be true before calling lens_update so the
  // active strategy's parameter handles are available before the first frame.
  assert(atomic_load_explicit(&filter->params_cached, memory_order_acquire));

  lens_update(filter,
              settings); // lens_update handles the addref and assignment

  emulens_inc_instances();
  EMULENS_LOG_OK("Filter instance created (mode=%d)", filter->mode);
  return filter;
}

static void lens_destroy(void *data) {
  struct lens_data *filter = data;
  if (!filter)
    return;

  // Signal to lens_update that teardown is in flight.
  atomic_store_explicit(&filter->destroying, true, memory_order_release);

  // Match lens_update's graphics -> render_mutex order. Enter graphics before
  // waiting for the render lock so teardown cannot deadlock with an update.
  obs_enter_graphics();
  pthread_mutex_lock(&filter->render_mutex);
  pthread_mutex_lock(&filter->settings_mutex);
  if (filter->cached_settings) {
    obs_data_release(filter->cached_settings);
    filter->cached_settings = NULL;
  }
  pthread_mutex_unlock(&filter->settings_mutex);

  // Only tear down the active mode's resources. An instance never
  // activates more than one mode, so calling all N destroy* would be
  // both wasteful and incorrect (double-free or cross-mode leaks).
  int active_mode =
      (int)atomic_load_explicit(&filter->mode, memory_order_acquire);
  if (active_mode >= 0 && active_mode < NUM_MODES) {
    if (strategies[active_mode].destroy_cpu)
      strategies[active_mode].destroy_cpu(filter);
  }

  if (active_mode >= 0 && active_mode < NUM_MODES) {
    if (strategies[active_mode].destroy)
      strategies[active_mode].destroy(filter);
  }
  safe_texrender_destroy(&filter->texrenders[0]);
  safe_texrender_destroy(&filter->texrenders[1]);
  safe_texrender_destroy(&filter->vidicon_history[0]);
  safe_texrender_destroy(&filter->vidicon_history[1]);
  safe_texrender_destroy(&filter->blur_h_buffer);
  safe_texrender_destroy(&filter->blur_v_buffer);
  safe_texrender_destroy(&filter->input_texrender);
  safe_texrender_destroy(&filter->output_texrender);
  safe_texrender_destroy(&filter->bloom_anam_h);
  safe_texrender_destroy(&filter->bloom_anam_v);
  destroy_bloom_pyramid_buffers(filter);
  if (filter->noise_tex) {
    gs_texture_destroy(filter->noise_tex);
    filter->noise_tex = NULL;
  }
  if (filter->effect) {
    gs_effect_destroy(filter->effect);
    filter->effect = NULL;
  }
  pthread_mutex_unlock(&filter->render_mutex);
  obs_leave_graphics();
  pthread_mutex_destroy(&filter->settings_mutex);
  pthread_mutex_destroy(&filter->render_mutex);
  emulens_dec_instances();
  bfree(filter);
}

// -------------------------------------------------------------------------------------------------
// #11 - Rendering Helpers (Decomposed lens_render)
// -------------------------------------------------------------------------------------------------
//
// capture_input_texture was inlined here. It has been replaced by
// get_input_source() in src/obs-utils.c, which adds sRGB color-space
// awareness via obs_source_process_filter_begin_with_color_space.

void bind_common_uniforms(struct lens_data *filter, gs_texture_t *source_tex,
                          uint32_t width, uint32_t height) {
  float rt_w = (float)width;
  float rt_h = (float)height;

  if (filter->common.elapsed_time) {
    float elapsed_sec =
        (float)emulens_atomic_load(&filter->elapsed_time_ns) / 1000000000.0f;
    SET_PARAM_FLOAT(filter->common.elapsed_time, elapsed_sec);
  }

  struct vec4 uv_pixel_interval;
  vec4_set(&uv_pixel_interval, 1.0f / rt_w, 1.0f / rt_h, 0.0f, 0.0f);
  if (filter->common.uv_pixel_interval)
    SET_PARAM_VEC4(filter->common.uv_pixel_interval, &uv_pixel_interval);

  if (filter->common.main_tex)
    SET_PARAM_TEXTURE(filter->common.main_tex, source_tex);
  if (filter->common.noise_tex)
    SET_PARAM_TEXTURE(filter->common.noise_tex, filter->noise_tex);
}

void bind_common_parameters(struct lens_data *filter, gs_texture_t *source_tex,
                            uint32_t width, uint32_t height) {
  float rt_w = (float)width;
  float rt_h = (float)height;

  /*
   * Internal texrender passes own their local projection/model state.  The
   * final composite must NOT call this helper: OBS has already established the
   * scene-item transform for the source, and resetting it here makes a
   * filtered 1280x720 source draw as though it were the 1920x1080 canvas.
   */
  gs_ortho(0.0f, rt_w, 0.0f, rt_h, -100.0f, 100.0f);
  gs_matrix_identity();

  bind_common_uniforms(filter, source_tex, width, height);
}

static void lens_render(void *data, gs_effect_t *effect_in) {
  UNUSED_PARAMETER(effect_in);
  if (!data)
    return;
  struct lens_data *filter = data;

  // Guard: context destroyed or teardown in flight — must skip, not deref.
  if (!filter->context)
    return;
  if (atomic_load_explicit(&filter->destroying, memory_order_acquire)) {
    obs_source_skip_video_filter(filter->context);
    return;
  }
  if (!gs_get_context()) {
    obs_source_skip_video_filter(filter->context);
    return;
  }

  pthread_mutex_lock(&filter->render_mutex);

  gs_effect_t *effect = filter->effect;
  if (!effect) {
    pthread_mutex_unlock(&filter->render_mutex);
    obs_source_skip_video_filter(filter->context);
    return;
  }
  if (!filter->param_validation.critical_params_valid) {
    // Re-try validation every ~120 frames in case GPU context recovered
    // from a transient allocation failure at startup.
    filter->param_validation.retry_count =
        (filter->param_validation.retry_count + 1) % 120;
    if (filter->param_validation.retry_count == 0) {
      filter->param_validation.validation_complete = false;
      recache_params_after_compile(filter);
      validate_param_availability(filter);
    }
    pthread_mutex_unlock(&filter->render_mutex);
    obs_source_skip_video_filter(filter->context);
    return;
  }

  // GPU capability detection (one-shot, deferred to render thread for
  // guaranteed device context).
  detect_gpu_capabilities(filter);

  // 1. STABLE DIMENSIONS
  uint32_t base_w = 0;
  uint32_t base_h = 0;
  if (!lens_get_render_size(filter, &base_w, &base_h) || base_w == 0 ||
      base_h == 0) {
    pthread_mutex_unlock(&filter->render_mutex);
    obs_source_skip_video_filter(filter->context);
    return;
  }
  uint32_t target_w = base_w;
  uint32_t target_h = base_h;

  // 2a. Check thrash-guard suspension before attempting reallocation.
  if (filter->render_suspended_until > 0) {
    if (os_gettime_ns() < filter->render_suspended_until) {
      pthread_mutex_unlock(&filter->render_mutex);
      obs_source_skip_video_filter(filter->context);
      return;
    }
    filter->render_suspended_until = 0;
    filter->consecutive_thrash = 0;
    filter->thrash_backoff_ms = 0;
  }

  // 2b. Ensure internal GPU buffers match the stable base dimensions.
  if (base_w != filter->allocated_width || base_h != filter->allocated_height) {
    texrender_result_t res =
        recreate_texrenders_if_needed(filter, base_w, base_h);
    if (res == TEXRENDER_FAILED) {
      if (filter->thrash_backoff_ms == 0)
        filter->thrash_backoff_ms = 200;
      uint64_t backoff_ns = (uint64_t)filter->thrash_backoff_ms * 1000000ULL;
      filter->thrash_backoff_ms = (filter->thrash_backoff_ms > 2500)
                                      ? 5000
                                      : filter->thrash_backoff_ms * 2;
      filter->render_suspended_until = os_gettime_ns() + backoff_ns;
      pthread_mutex_unlock(&filter->render_mutex);
      obs_source_skip_video_filter(filter->context);
      return;
    }
  }

  // 3. Capture the current source texture at base resolution

  if (!get_input_source(filter, base_w, base_h)) {
    pthread_mutex_unlock(&filter->render_mutex);
    obs_source_skip_video_filter(filter->context);
    return;
  }
  gs_texture_t *source_tex = gs_texrender_get_texture(filter->input_texrender);
  if (!source_tex) {
    pthread_mutex_unlock(&filter->render_mutex);
    obs_source_skip_video_filter(filter->context);
    return;
  }

  // Snapshot mode atomically to ensure consistency across the frame.
  int mode = atomic_load_explicit(&filter->mode, memory_order_acquire);

  // 4. Internal Strategy Rendering (at stable base resolution)
  // Re-bind params from cached settings every frame — shader params
  // may not persist between frames on all OBS/GPU backends.
  gs_texture_t *final_tex = NULL;
  if (mode >= 0 && mode < NUM_MODES) {
    const lens_effect_vtable_t *strat = &strategies[mode];
    if (strat && strat->render) {
      // Acquire cached settings for param re-binding.
      obs_data_t *settings = NULL;
      pthread_mutex_lock(&filter->settings_mutex);
      if (filter->cached_settings) {
        settings = filter->cached_settings;
        obs_data_addref(settings);
      }
      pthread_mutex_unlock(&filter->settings_mutex);
      if (settings && strat->update_settings)
        strat->update_settings(filter, settings);
      if (settings)
        obs_data_release(settings);
      gs_viewport_push();
      gs_set_viewport(0, 0, (int)base_w, (int)base_h);
      gs_projection_push(); // protect OBS's projection for restore
      gs_matrix_push();     // protect OBS's matrix for restore

      final_tex =
          strat->render(filter, filter->effect, source_tex, base_w, base_h);
      // bind_common_parameters (called by each strategy) sets gs_ortho +
      // gs_matrix_identity

      if (!final_tex) {
        EMULENS_LOG(LOG_DEBUG, "Strategy render returned NULL for mode %d",
                    mode);
      }

      gs_matrix_pop();
      gs_projection_pop();
      gs_viewport_pop();
    }
  }

  // 5. COMPOSITE: Draw the strategy result to the current render target.
  // Always normalize blend state before drawing, regardless of whether
  // a strategy produced an intermediate texture.  Upstream blend state
  // must never leak into the plugin's output.
  gs_texture_t *composite_tex = final_tex ? final_tex : source_tex;
  bool composite_ok = false;
  if (composite_tex) {
    /*
     * The strategy output is already complete.  Composite it back into OBS
     * with OBS's own base Draw effect instead of routing the finished texture
     * through EmuLens's custom shader a second time.  The custom Draw path can
     * silently produce black on OpenGL even when the strategy pass succeeded.
     *
     * Preserve OBS's current scene-item projection/model transform here; the
     * sprite helper only supplies the source-local quad.
     */
    gs_effect_t *copy_effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
    gs_technique_t *copy_tech =
        copy_effect ? gs_effect_get_technique(copy_effect, "Draw") : NULL;
    gs_eparam_t *image_param =
        copy_effect ? gs_effect_get_param_by_name(copy_effect, "image") : NULL;

    if (copy_tech && image_param) {
      /*
       * Mirror libobs render_filter_tex() exactly.  In particular, bind the
       * texture before beginning the pass and pass the real texture to
       * gs_draw_sprite(); setting the image parameter after begin_pass is too
       * late on some OpenGL paths.
       */
      const bool linear_srgb = gs_get_linear_srgb();
      const bool previous_srgb = gs_framebuffer_srgb_enabled();
      gs_enable_framebuffer_srgb(linear_srgb);

      if (linear_srgb)
        gs_effect_set_texture_srgb(image_param, composite_tex);
      else
        gs_effect_set_texture(image_param, composite_tex);

      size_t passes = gs_technique_begin(copy_tech);
      if (passes > 0) {
        composite_ok = true;
        for (size_t i = 0; i < passes; ++i) {
          if (!gs_technique_begin_pass(copy_tech, i)) {
            composite_ok = false;
            break;
          }
          gs_draw_sprite(composite_tex, 0, target_w, target_h);
          gs_technique_end_pass(copy_tech);
        }
        gs_technique_end(copy_tech);
      }

      gs_enable_framebuffer_srgb(previous_srgb);
    }
  }

  pthread_mutex_unlock(&filter->render_mutex);
  if (!composite_ok)
    obs_source_skip_video_filter(filter->context);
}

static enum gs_color_space
lens_get_color_space(void *data, size_t count,
                     const enum gs_color_space *preferred_spaces) {
  struct lens_data *filter = data;
  if (!filter || !filter->context)
    return (count > 0) ? preferred_spaces[0] : GS_CS_SRGB;

  const enum gs_color_space potential_spaces[] = {
      GS_CS_SRGB,
      GS_CS_SRGB_16F,
      GS_CS_709_EXTENDED,
  };

  /*
   * Follow the next target in the filter chain, matching OBS's built-in
   * filters.  Then honor the caller's preferred-space list when possible.
   */
  obs_source_t *target = obs_filter_get_target(filter->context);
  if (!target)
    return (count > 0) ? preferred_spaces[0] : GS_CS_SRGB;

  enum gs_color_space source_space =
      obs_source_get_color_space(target, OBS_COUNTOF(potential_spaces),
                                 potential_spaces);
  enum gs_color_space space = source_space;
  for (size_t i = 0; i < count; ++i) {
    space = preferred_spaces[i];
    if (space == source_space)
      break;
  }
  return space;
}

struct obs_source_info emulens_filter_info = {
    .id = "emulens_fx",
    .type = OBS_SOURCE_TYPE_FILTER,
    .output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW,
    .get_name = lens_get_name,
    .create = lens_create,
    .destroy = lens_destroy,
    .video_tick = lens_tick,
    .video_render = lens_render,
    .get_properties = emulens_properties,
    .get_defaults = emulens_get_defaults,
    .update = lens_update,
    .video_get_color_space = lens_get_color_space,
};
