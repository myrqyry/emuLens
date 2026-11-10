/**
 * @file strategies.c
 * @brief Strategy VTable Definition and Global Strategy Array
 *
 * This file includes all individual strategy implementations and
 * defines the global strategies[] array used by lens-core.c.
 *
 * #4 - File Organization Refactor
 * Instead of one monolithic 673-line file, strategies are now
 * organized into individual files for better maintainability.
 */

/*
 * REQUIRED BUILD SOURCES — all strategy-*.c files must be listed in CMakeLists.txt:
 *   strategy-vidicon.c, strategy-starburst.c, strategy-leak.c, strategy-handheld.c,
 *   strategy-bokeh.c, strategy-particles.c, strategy-orton.c, strategy-grit.c,
 *   strategy-mirage.c, strategy-spot.c, strategy-bloom.c, strategy-pinhole.c,
 *   strategy-polaroid.c, strategy-daguerreotype.c, strategy-technicolor.c,
 *   strategy-wet-lens.c, strategy-phone-wet.c
 *
 * Adding a new mode: (1) add X(...) to EMULENS_MODES in lens-data.h,
 * (2) create strategy-NAME.c, (3) declare functions in strategies.h,
 * (4) add source to CMakeLists.txt STRATEGY_SOURCES.
 */
#include "strategies.h"
#include "../lens-data.h"

// -------------------------------------------------------------------------------------------------
// Global Strategy Table (Generated via X-Macro)
// -------------------------------------------------------------------------------------------------
#define X(id, suffix, display_name, tech)                                      \
  [MODE_##id] = {suffix##_cache, suffix##_update, suffix##_render,              \
                 suffix##_get_defaults, suffix##_destroy_cpu, suffix##_destroy, \
                 suffix##_populate_presets, tech},

const lens_effect_vtable_t strategies[NUM_MODES] = {EMULENS_MODES};

#undef X
