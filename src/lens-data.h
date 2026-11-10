#pragma once

#include <stddef.h>
#include <stdint.h>

// Portable atomic helpers: C11 <stdatomic.h> in C, GCC builtins in C++
// (C++ test compilation can't include <atomic> inside extern "C" {})
#ifdef __cplusplus
// C++ uses GCC/Clang __atomic builtins (same as original code, still portable)
#define emulens_atomic_load(ptr)    __atomic_load_n(ptr, __ATOMIC_ACQUIRE)
#define emulens_atomic_store(ptr,v) __atomic_store_n(ptr, v, __ATOMIC_RELEASE)
#else
#include <stdatomic.h>
#define emulens_atomic_load(ptr)                                               \
  atomic_load_explicit((ptr), memory_order_acquire)
#define emulens_atomic_store(ptr, val)                                         \
  atomic_store_explicit((ptr), (val), memory_order_release)
#endif

// ──────────────────────────────────────────────────────────────────────────────
//  Enhanced Logging Helpers  (shared across all translation units)
// ──────────────────────────────────────────────────────────────────────────────

/* Log prefix used by all emuLensFX log lines */
#define LOG_TAG "[EmuLensFX]"

/* Shortcut: blog with our tag */
#define EMULENS_LOG(level, fmt, ...)                                           \
  blog(level, LOG_TAG " " fmt, ##__VA_ARGS__)

/* ── Status emoji helpers for glanceability ── */
#define EMULENS_LOG_OK(fmt, ...)                                               \
  EMULENS_LOG(LOG_INFO, u8"🟢 " fmt, ##__VA_ARGS__)
#define EMULENS_LOG_WARN(fmt, ...)                                             \
  EMULENS_LOG(LOG_WARNING, u8"🟡 " fmt, ##__VA_ARGS__)
#define EMULENS_LOG_ERROR(fmt, ...)                                            \
  EMULENS_LOG(LOG_ERROR, u8"🔴 " fmt, ##__VA_ARGS__)
#define EMULENS_LOG_CONFIG(fmt, ...)                                           \
  EMULENS_LOG(LOG_INFO, u8"⚙️ " fmt, ##__VA_ARGS__)

/* ── Error block ── */
#define EMULENS_LOG_CRITICAL_BLOCK(reason, action)                             \
  do {                                                                         \
    EMULENS_LOG(LOG_ERROR,                                                     \
                "===================================================");        \
    EMULENS_LOG(LOG_ERROR,                                                     \
                "  !!! PLUGIN CRITICAL FAILURE !!!");                          \
    EMULENS_LOG(LOG_ERROR,                                                     \
                "---------------------------------------------------");        \
    EMULENS_LOG(LOG_ERROR, "  Reason: %s", reason);                           \
    EMULENS_LOG(LOG_ERROR, "  Action: %s", action);                           \
    EMULENS_LOG(LOG_ERROR,                                                     \
                "===================================================");        \
  } while (0)

// ──────────────────────────────────────────────────────────────────────────────
//  End Enhanced Logging Helpers
// ──────────────────────────────────────────────────────────────────────────────

#include <graphics/effect.h>
#include <graphics/graphics.h>
#include <graphics/matrix4.h>
#include <graphics/vec2.h>
#include <graphics/vec4.h>
#include <math.h>
#include <obs-module.h>
#include <stdbool.h>
#include <util/dstr.h>
#include <util/platform.h>
#include <util/threading.h>

// Portable atomic type aliases (must follow <stdbool.h>/<stdatomic.h> includes)
#ifdef __cplusplus
typedef int64_t emulens_atomic_i64;
typedef int emulens_atomic_int;
typedef bool emulens_atomic_bool;
#else
typedef _Atomic int64_t emulens_atomic_i64;
typedef _Atomic int emulens_atomic_int;
typedef _Atomic bool emulens_atomic_bool;
#endif

// Plugin version — CMake defines this via buildspec.json; fallback for dev
#ifndef PLUGIN_VERSION
#define PLUGIN_VERSION "dev"
#endif

struct lens_data;

// Lifecycle external functions
void emulens_inc_instances(void);
void emulens_dec_instances(void);

// Custom Draw Helper to bypass gs_draw_sprite parameter lookup
void lens_draw_quad(uint32_t cx, uint32_t cy);

// Safe Parameter Retrieval
#define GET_PARAM_SAFE(var, effect, name)                                      \
  do {                                                                         \
    var = gs_effect_get_param_by_name(effect, name);                           \
    if (!var) {                                                                \
      EMULENS_LOG(LOG_DEBUG, "Shader parameter '%s' not found (expected "      \
                   "when not in active mode)", name);                           \
    }                                                                          \
  } while (0)

#include "lens-params.h"

#ifndef SET_PARAM_FLOAT
#error "SET_PARAM_FLOAT not defined — check lens-params.h is included correctly"
#endif

// -------------------------------------------------------------------------------------------------
// Mode Definitions (Generated via X-Macro)
// -------------------------------------------------------------------------------------------------
#define X(id, suffix, display_name, tech_name) MODE_##id,
enum emulens_mode { EMULENS_MODES NUM_MODES };
#undef X

// Performance and safety constants
#define TIME_WRAP_THRESHOLD 7200.0f // 2 hours - prevents float precision loss
#define DIMENSION_CHECK_INTERVAL 10 // Check dimensions every N frames
#define DIMENSION_CHECK_FPS_EQUIVALENT 6.0f // At 60fps = 6 checks/second
#define MAX_BLUR_SAMPLES 32 // Maximum Gaussian blur samples
#define BLOOM_PYRAMID_LEVELS 4
#define MIN_BLUR_RADIUS 0.1f
#define MAX_BLUR_RADIUS 50.0f
#define MIN_VALID_DIMENSION 1                // Minimum valid width/height
#define TEXRENDER_CLEAR_SIZE 1               // Size for clearing texrenders
#define NOISE_TEX_SIZE 256

// -------------------------------------------------------------------------------------------------
// Parameter Structures
// -------------------------------------------------------------------------------------------------

typedef struct {
  gs_eparam_t *history_image;
  gs_eparam_t *vid_decay;
  gs_eparam_t *vid_threshold;
  gs_eparam_t *vid_color;
  gs_eparam_t *vid_blend;
  gs_eparam_t *vid_red_lag;
  gs_eparam_t *vid_blue_lag;
  gs_eparam_t *vid_comet;
  gs_eparam_t *vid_comet_length;
  gs_eparam_t *vid_noise;
  gs_eparam_t *vid_beam_pull;
  gs_eparam_t *vid_lag_nonlinear;
  gs_eparam_t *vid_register;
  gs_eparam_t *vid_register_drift;
  gs_eparam_t *vid_scanlines;
  gs_eparam_t *vid_gamma;
  gs_eparam_t *smear_enable;
  gs_eparam_t *smear_mode;
  gs_eparam_t *smear_direction;
  gs_eparam_t *smear_strength;
  gs_eparam_t *vid_halation;
  gs_eparam_t *vid_hum;
  gs_eparam_t *vid_source_color;
} vidicon_params_t;

typedef struct {
  gs_eparam_t *threshold;
  gs_eparam_t *intensity;
  gs_eparam_t *points;
  gs_eparam_t *ray_length;
  gs_eparam_t *thickness;
  gs_eparam_t *smoothness;
  gs_eparam_t *colorize;
  gs_eparam_t *color;
  gs_eparam_t *chroma;
  gs_eparam_t *rot_speed;
  gs_eparam_t *rotation;
  gs_eparam_t *pulse_speed;
  gs_eparam_t *pulse_amp;
  gs_eparam_t *samples;
  gs_eparam_t *anamorphic;
  gs_eparam_t *blend_mode;
} starburst_params_t;

typedef struct {
  gs_eparam_t *intensity;
  gs_eparam_t *speed;
  gs_eparam_t *scale;
  gs_eparam_t *color;
  gs_eparam_t *color_enable_shift;
  gs_eparam_t *color_shift;
  gs_eparam_t *color_speed;
  gs_eparam_t *bias_top;
  gs_eparam_t *bias_bot;
  gs_eparam_t *bias_left;
  gs_eparam_t *bias_right;
  gs_eparam_t *edge_falloff;
  gs_eparam_t *streakiness;
  gs_eparam_t *contrast;
  gs_eparam_t *complexity;
  gs_eparam_t *blend_mode;
} leak_params_t;

typedef struct {
  gs_eparam_t *pos_amp;
  gs_eparam_t *pos_speed;
  gs_eparam_t *rot_amp;
  gs_eparam_t *rot_speed;
  gs_eparam_t *zoom_amp;
  gs_eparam_t *zoom_speed;
  gs_eparam_t *pos_offset;
  gs_eparam_t *rot_rad;
  gs_eparam_t *zoom_factor;
  gs_eparam_t *blur_enable;
  gs_eparam_t *blur_amp;
  gs_eparam_t *blur_speed;
  gs_eparam_t *blur_static;
  gs_eparam_t *feather;
  gs_eparam_t *vel_dir_x;
  gs_eparam_t *vel_dir_y;
  gs_eparam_t *vel_mag;
  gs_eparam_t *dir_blur_enable;
  gs_eparam_t *blur_directional;
} handheld_params_t;

typedef struct {
  gs_eparam_t *radius;
  gs_eparam_t *power;
  gs_eparam_t *shape;
  gs_eparam_t *focus_x;
  gs_eparam_t *focus_y;
  gs_eparam_t *width;
  gs_eparam_t *falloff;
  gs_eparam_t *boost;
  gs_eparam_t *high_response;
  gs_eparam_t *threshold;
} bokeh_params_t;

typedef struct {
  gs_eparam_t *density;
  gs_eparam_t *size;
  gs_eparam_t *size_var;
  gs_eparam_t *speed;
  gs_eparam_t *wind_x;
  gs_eparam_t *wind_y;
  gs_eparam_t *wobble;
  gs_eparam_t *color_start;
  gs_eparam_t *color_end;
  gs_eparam_t *source_color;
  gs_eparam_t *source_influence;
  gs_eparam_t *source_brightness;
  gs_eparam_t *source_threshold;
  gs_eparam_t *source_strength;
  gs_eparam_t *focus_x;
  gs_eparam_t *focus_y;
  gs_eparam_t *focus_strength;
  gs_eparam_t *blur_edge;
  gs_eparam_t *use_poly;
  gs_eparam_t *poly_sides;
  gs_eparam_t *poly_rot;
  gs_eparam_t *poly_rot_speed;
  gs_eparam_t *part_ca_strength;
} particles_params_t;

typedef struct {
  gs_eparam_t *opacity;
  gs_eparam_t *contrast;
  gs_eparam_t *brightness;
  gs_eparam_t *saturation;
  gs_eparam_t *blend_mode;
  gs_eparam_t *highlight_boost;
} orton_params_t;

typedef struct {
  gs_eparam_t *threshold;
  gs_eparam_t *intensity;
  gs_eparam_t *radius;
  gs_eparam_t *softness;
  gs_eparam_t *dust_amount;
  gs_eparam_t *dust_scale;
  gs_eparam_t *haze_amount;
  gs_eparam_t *dust_contrast;
  gs_eparam_t *refract_amount;
  gs_eparam_t *refract_scale;
  gs_eparam_t *highlights_only;
  gs_eparam_t *anamorphic;
  gs_eparam_t *color;
  gs_eparam_t *dust_specular;
  gs_eparam_t *dust_depth_layers;
  gs_eparam_t *ghost_amount;
  gs_eparam_t *ghost_scale;
  gs_eparam_t *ghost_color;
} bloom_params_t;

typedef struct {
  gs_eparam_t *density;
  gs_eparam_t *size;
  gs_eparam_t *refraction;
  gs_eparam_t *glint;
  gs_eparam_t *stretch;
  gs_eparam_t *chromatic;
  gs_eparam_t *seed;
} wet_lens_params_t;

typedef struct {
  gs_eparam_t *coverage;
  gs_eparam_t *scale;
  gs_eparam_t *refraction;
  gs_eparam_t *ribbons;
  gs_eparam_t *reach;
  gs_eparam_t *glow;
  gs_eparam_t *arcs;
  gs_eparam_t *angle;
  gs_eparam_t *seed;
} phone_wet_params_t;

typedef struct {
  gs_eparam_t *dust;
  gs_eparam_t *scratches;
  gs_eparam_t *noise;
  gs_eparam_t *flares;
  gs_eparam_t *color;
  gs_eparam_t *brightness;
  gs_eparam_t *contrast;
  gs_eparam_t *saturation;
  gs_eparam_t *opacity;
  gs_eparam_t *instability;
  gs_eparam_t *blend_mode;
  gs_eparam_t *scratch_type;
  gs_eparam_t *dust_size;
} grit_params_t;


typedef struct {
  gs_eparam_t *count;
  gs_eparam_t *radius;
  gs_eparam_t *rotation;
  gs_eparam_t *rot_speed;
  gs_eparam_t *center_mask;
  gs_eparam_t *parallel;
} mirage_params_t;

typedef struct {
  gs_eparam_t *softness;
  gs_eparam_t *vignette;
  gs_eparam_t *distortion;
  gs_eparam_t *sepia;
  gs_eparam_t *grain;
} pinhole_params_t;

typedef struct {
  gs_eparam_t *softness;
  gs_eparam_t *vignette;
  gs_eparam_t *color_shift;
  gs_eparam_t *contrast;
  gs_eparam_t *grain;
  gs_eparam_t *develop;
  gs_eparam_t *develop_center;
  gs_eparam_t *base_color;
  gs_eparam_t *bleed;
} polaroid_params_t;

typedef struct {
  gs_eparam_t *contrast;
  gs_eparam_t *grain;
  gs_eparam_t *tint;
  gs_eparam_t *vignette;
  gs_eparam_t *edge_blur;
} daguerreotype_params_t;

typedef struct {
  gs_eparam_t *strength;
  gs_eparam_t *saturation;
  gs_eparam_t *offset;
  gs_eparam_t *glow;
} technicolor_params_t;

typedef struct {
  gs_eparam_t *radius;
  gs_eparam_t *softness;
  gs_eparam_t *zoom_strength;
  gs_eparam_t *blur_amount;
  gs_eparam_t *blur_radius;
  gs_eparam_t *tint;
  gs_eparam_t *blend_mode;
} spot_params_t;

// -------------------------------------------------------------------------------------------------
// Main Filter Data Structure
// -------------------------------------------------------------------------------------------------

struct lens_data {
  // Effect
  gs_effect_t *effect;
  obs_source_t *context;

  // Cached settings for render-thread param binding
  obs_data_t *cached_settings;
  pthread_mutex_t settings_mutex;
  pthread_mutex_t render_mutex;

  // Allocated Dimensions (placed here to avoid memory corruption from unions)
  uint32_t allocated_width;
  uint32_t allocated_height;
  emulens_atomic_int last_source_width;
  emulens_atomic_int last_source_height;
  emulens_atomic_int source_miss_count; // staleness counter for cached source dimensions

  // Parameter handles
  struct {
    gs_eparam_t *elapsed_time;
    gs_eparam_t *uv_pixel_interval;
    gs_eparam_t *noise_tex;
    gs_eparam_t *main_tex;
    gs_eparam_t *ViewProj;
    gs_eparam_t *blur_radius;
    gs_eparam_t *blurred_image;
  } common;

  // Technique handles
  struct {
    gs_technique_t *draw;
    gs_technique_t *blur_h;
    gs_technique_t *blur_v;
    gs_technique_t *vidicon;
    gs_technique_t *vidicon_composite;
    gs_technique_t *starburst;
    gs_technique_t *leak;
    gs_technique_t *handheld;
    gs_technique_t *bokeh;
    gs_technique_t *particles;
    gs_technique_t *orton;
    gs_technique_t *bloom_extract;
    gs_technique_t *bloom_downsample;
    gs_technique_t *bloom_upsample;
    gs_technique_t *bloom_combine;
    gs_technique_t *grit;
    gs_technique_t *mirage;
    gs_technique_t *spot;
    gs_technique_t *pinhole;
    gs_technique_t *polaroid;
    gs_technique_t *daguerreotype;
    gs_technique_t *technicolor;
    gs_technique_t *wet_lens;
    gs_technique_t *phone_wet;
  } tech;

  // Union: only one mode is active at a time (enforced by the vtable/strategy
  // pattern), so all effect parameter structs can share one slot. This enforces the
  // single-active contract at the type level and shrinks the instance.
  union {
    vidicon_params_t vidicon;
    starburst_params_t starburst;
    leak_params_t leak;
    handheld_params_t handheld;
    bokeh_params_t bokeh;
    particles_params_t particles;
    orton_params_t orton;
    bloom_params_t bloom;
    grit_params_t grit;
    mirage_params_t mirage;
    spot_params_t spot;
    pinhole_params_t pinhole;
    polaroid_params_t polaroid;
    daguerreotype_params_t daguerreotype;
    technicolor_params_t technicolor;
    wet_lens_params_t wet_lens;
    phone_wet_params_t phone_wet;
  } mode_params;

  emulens_atomic_bool params_cached;


  // Buffers
  // Texrender slot contract (shared pool — only one mode active at a time):
  //   Slot 0: Output buffer for non-feedback modes (bloom, starburst, orton,
  //           handheld, etc.), also vidicon trail_buf (cur_write).
  //   Slot 1: Scratch/extract buffer for non-feedback modes (bloom extract),
  //           also vidicon history_buf ((cur_write+1)%2).
  // Vidicon manages cur_write (ping-pong 0↔1 each frame) for feedback.
  // Non-feedback modes must NOT mutate cur_write.
  gs_texrender_t *texrenders[2];
  emulens_atomic_int cur_write;
  gs_texrender_t *vidicon_history[2]; // Dedicated feedback buffers
  gs_texrender_t *blur_h_buffer;
  gs_texrender_t *blur_v_buffer;
  gs_texrender_t *input_texrender;
  gs_texrender_t *output_texrender; /* off-screen render at base size, then scaled to viewport */
  // Dedicated bloom anamorphic blur buffers (not shared with apply_gaussian_blur)
  gs_texrender_t *bloom_anam_h;
  gs_texrender_t *bloom_anam_v;
  // Alpha-Piscium/Jimenez-style multi-resolution bloom. Downsample levels are
  // RGBA16F so additive reconstruction keeps highlight energy above 1.0.
  gs_texrender_t *bloom_pyramid[BLOOM_PYRAMID_LEVELS];
  gs_texrender_t *bloom_upsample[BLOOM_PYRAMID_LEVELS - 1];

  // State
  emulens_atomic_int mode;
  emulens_atomic_i64 elapsed_time_ns;

  // Runtime State Storage — namespaced per owning mode so a typo becomes a
  // compile error instead of a silent cross-mode write. Only one mode is
  // active per frame, so the members may share storage (union).
  union {
    struct {                 // Bloom
      float radius;
      bool anamorphic;
    } bloom;
    struct {                 // Orton
      float spread;
      float prev_opacity;
      float prev_contrast;
      float prev_brightness;
      float prev_saturation;
      float prev_blend_mode;
      float prev_highlight_boost;
    } orton;
    struct {                 // Vidicon
      float user_decay;
      float prev_elapsed_sec;
      bool time_initialized;
    } vidicon;
    struct {                 // Handheld
      float pos_amp;
      float pos_speed;
      float rot_amp;
      float rot_speed;
      float zoom_amp;
      float zoom_speed;
      float blur_static;
      bool blur_enable;
      float blur_amp;
      float blur_speed;
      float tremor_amp;
      float breath_amp;
      float step_amp;
      float step_speed;
      float correction_amp;
      float inertia;
      float blur_response;
      struct vec2 smoothed_offset;
      struct vec2 prev_offset;
      float smoothed_rot_rad;
      float prev_rot_rad;
      float smoothed_zoom_factor;
      float prev_zoom_factor;
      float prev_elapsed_sec;
      float blur_velocity;
      bool motion_initialized;
      float phase_seed;      // random offset to prevent wrap snap
    } handheld;
  } runtime_state;

  // GPU Capabilities Detection (#6 - adaptive rendering based on hardware)
  struct {
    bool high_precision_float;
    int max_texture_size;
    bool capabilities_detected;
  } gpu_caps;

  // Destructor guard: set before teardown so lens_update can bail out
  emulens_atomic_bool destroying;

  // Parameter validation tracking
  struct {
    bool validation_complete;
    bool critical_params_valid;
    uint32_t retry_count; // periodic re-validation when critical_params_valid==false
  } param_validation;

  // (UI state reserved for future use)

  // Caching vars
  uint64_t last_realloc_time;     // Safety check against thrashing
  uint32_t consecutive_thrash;    // FIX: Moved from static to instance struct

  // Thrash guard recovery: non-zero = suspend rendering until this timestamp
  uint64_t render_suspended_until;
  // Exponential backoff for thrash guard: doubles each failure, caps at 5s.
  uint32_t thrash_backoff_ms;

  // Config

  // Generated noise texture
  gs_texture_t *noise_tex;

  // Per-instance HDR/PQ warning flag (instance-level prevents cross-instance
  // suppression that a static bool would cause).
  bool hdr_warned;
};

// -------------------------------------------------------------------------------------------------
// Shared Functions
// -------------------------------------------------------------------------------------------------

gs_texture_t *apply_gaussian_blur(struct lens_data *filter,
                                   gs_texture_t *tex, uint32_t w, uint32_t h,
                                   float radius);

void bind_common_uniforms(struct lens_data *filter, gs_texture_t *source_tex,
                          uint32_t width, uint32_t height);
void bind_common_parameters(struct lens_data *filter, gs_texture_t *source_tex,
                            uint32_t width, uint32_t height);

// UI Builders
obs_properties_t *emulens_properties(void *data);
void emulens_get_defaults(obs_data_t *settings);

// Strategy Interface
typedef struct {
  void (*cache_params)(struct lens_data *filter, gs_effect_t *effect);
  void (*update_settings)(struct lens_data *filter, obs_data_t *settings);
  gs_texture_t *(*render)(struct lens_data *filter, gs_effect_t *effect,
                          gs_texture_t *source_tex, uint32_t width,
                          uint32_t height);
  void (*get_defaults)(obs_data_t *settings);
  void (*destroy_cpu)(struct lens_data *filter);
  void (*destroy)(struct lens_data *filter);
  void (*populate_presets)(obs_property_t *list);
  const char *tech_name;
} lens_effect_vtable_t;

extern const lens_effect_vtable_t strategies[NUM_MODES];

// Inline helpers (clamp_int, clamp_float, lens_run_technique_quad) live
// in lens-inline.h. Including here keeps the include surface for .c files
// unchanged; splitting them out makes the boundary between struct-only
// declarations and shared implementations explicit.
#include "lens-inline.h"
