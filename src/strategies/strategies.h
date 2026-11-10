/**
 * @file strategies.h
 * @brief Strategy Function Declarations
 *
 * Forward declarations for all effect strategy functions.
 * Each strategy provides cache, update, and render functions.
 */

#ifndef LENS_STRATEGIES_H
#define LENS_STRATEGIES_H

/*
 * Resource ownership rules:
 *   - Shared lens_data texrenders, including vidicon_history[], effect,
 *     noise_tex, and common params are owned and destroyed by lens-core.c.
 *   - Any strategy-owned GPU resources (vertex buffers, per-strategy
 *     textures, custom texrenders) must be released in *_destroy().
 *   - Any strategy-owned CPU resources (allocated arrays, lookup tables,
 *     persistent state) must be released in *_destroy_cpu().
 *   - Empty destroy/destroy_cpu is correct only if the strategy allocates
 *     nothing outside the shared pool.
 */

#include "../lens-data.h"

// Auto-generated Strategy Declarations (cache / update / render / destroy_cpu / destroy)
#define X(id, suffix, display_name, tech)                                       \
  void suffix##_cache(struct lens_data *filter, gs_effect_t *effect);            \
  void suffix##_update(struct lens_data *filter, obs_data_t *settings);          \
  gs_texture_t *suffix##_render(struct lens_data *filter, gs_effect_t *effect,   \
                                gs_texture_t *source_tex, uint32_t width,        \
                                uint32_t height);                                \
  void suffix##_destroy_cpu(struct lens_data *filter);                           \
  void suffix##_destroy(struct lens_data *filter);
EMULENS_MODES
#undef X

// Auto-generated Get Defaults Declarations
#define X(id, suffix, display_name, tech) \
  void suffix##_get_defaults(obs_data_t *settings);
EMULENS_MODES
#undef X

// Auto-generated Populate Presets Declarations
#define X(id, suffix, display_name, tech) \
  void suffix##_populate_presets(obs_property_t *list);
EMULENS_MODES
#undef X

#endif // LENS_STRATEGIES_H
