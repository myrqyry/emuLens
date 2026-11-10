#include <errno.h>
#include <obs-module.h>
#include <util/platform.h>
#include <util/threading.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

/* Logging macros are shared from lens-data.h */
#include "lens-data.h"

extern struct obs_source_info emulens_filter_info;

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("emuLensFX", "en-US")

/**
 * @brief Returns the plugin description for OBS Plugin Manager (OBS 32+).
 * @return Human-readable description of the plugin's functionality.
 * @note Required by OBS plugin ABI - do not modify signature.
 */
MODULE_EXPORT const char *obs_module_description(void) {
  return "EmuLens - GPU camera, film, and optical effects for OBS Studio "
         "Effects";
}

/**
 * @brief Returns the display name shown in OBS Studio plugin list.
 * @return Plugin name as it appears in the UI.
 * @note Required by OBS plugin ABI - do not modify signature.
 */
MODULE_EXPORT const char *obs_module_name(void) { return "EmuLens"; }

/**
 * @brief Returns the plugin author's identifier.
 * @return Author name or handle.
 * @note Required by OBS plugin ABI - do not modify signature.
 */
MODULE_EXPORT const char *obs_module_author(void) { return "myrqyry"; }

// Global plugin state for lifecycle management.
// Fields are _Atomic so os_atomic_* operations are well-defined per C11.
static struct {
  emulens_atomic_bool initialized;
  emulens_atomic_int active_instances;
} plugin_state = {.initialized = false, .active_instances = 0};

// Exported for lens-core.c to use
void emulens_inc_instances(void) {
  if (!atomic_load_explicit(&plugin_state.initialized, memory_order_acquire))
    return;

  atomic_fetch_add_explicit(&plugin_state.active_instances, 1,
                            memory_order_acq_rel);
}

void emulens_dec_instances(void) {
  if (!atomic_load_explicit(&plugin_state.initialized, memory_order_acquire))
    return;

  int prev = atomic_fetch_sub_explicit(&plugin_state.active_instances, 1,
                                       memory_order_acq_rel);
  if (prev <= 0) {
    atomic_store_explicit(&plugin_state.active_instances, 0,
                          memory_order_release);
    EMULENS_LOG_WARN("Attempted to decrement instances below zero");
  }
}

/**
 * @brief Loads the plugin module.
 *
 * This function is called by OBS Studio when the plugin is loaded.
 * It prints a decorative startup banner and registers the source info.
 *
 * @return true if the module loaded successfully, false otherwise.
 */
bool obs_module_load(void) {
  /* ── OBS Version Guard ── */
  /* Keep this aligned with the OBS dependency declared in buildspec.json. */
#if LIBOBS_API_MAJOR_VER < 32 ||                                             \
    (LIBOBS_API_MAJOR_VER == 32 && LIBOBS_API_MINOR_VER < 1)
  EMULENS_LOG_ERROR("OBS %u.%u.%u detected — EmuLensFX requires OBS 32.1.0+",
                    LIBOBS_API_MAJOR_VER, LIBOBS_API_MINOR_VER,
                    LIBOBS_API_PATCH_VER);
  return false;
#endif

  bool expected = false;
  if (atomic_compare_exchange_strong_explicit(
          &plugin_state.initialized, &expected, true, memory_order_acq_rel,
          memory_order_acquire)) {
    /* ── Startup Banner ── */
    EMULENS_LOG_OK("EmuLensFX v%s loaded (OBS %u.%u.%u, %s %s)", PLUGIN_VERSION,
                   LIBOBS_API_MAJOR_VER, LIBOBS_API_MINOR_VER,
                   LIBOBS_API_PATCH_VER, __DATE__, __TIME__);

    obs_register_source(&emulens_filter_info);
  }

  return true;
}

/**
 * @brief Unloads the plugin module.
 *
 * This function is called by OBS Studio when the plugin is unloaded.
 * It performs necessary cleanup and checks for resource leaks.
 */
void obs_module_unload(void) {
  if (!atomic_load_explicit(&plugin_state.initialized, memory_order_acquire)) {
    return;
  }

  int active_instances = atomic_load_explicit(&plugin_state.active_instances,
                                              memory_order_acquire);

  if (active_instances > 0) {
    EMULENS_LOG_WARN("Unloading with %d active instance(s) — "
                     "potential resource leak",
                     active_instances);
  } else {
    EMULENS_LOG_OK("Unloading — no active instances remaining");
  }

  atomic_store_explicit(&plugin_state.initialized, false, memory_order_release);
  atomic_store_explicit(&plugin_state.active_instances, 0,
                        memory_order_release);

  EMULENS_LOG(LOG_INFO, "Plugin cleanup complete");
}
