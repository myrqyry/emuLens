/* Stub implementations for strategy vtable test.
 * These replace lens-core.c functions so test_strategies can
 * verify the strategy table without linking OBS graphics. */
#include "../src/lens-data.h"
#include <string.h>

void lens_draw_quad(uint32_t cx, uint32_t cy) { (void)cx; (void)cy; }
obs_data_t *lens_acquire_cached_settings(struct lens_data *filter) { (void)filter; return NULL; }
void bind_common_parameters(struct lens_data *filter, gs_texture_t *t, uint32_t w, uint32_t h)
{ (void)filter; (void)t; (void)w; (void)h; }
gs_texture_t *apply_gaussian_blur(struct lens_data *f, gs_texture_t *t, uint32_t w, uint32_t h, float r)
{ (void)f; (void)t; (void)w; (void)h; (void)r; return NULL; }

/* Stubs for OBS module API used by ADD_PRESET_ENTRY and obs_module_text.
 * obs_module_file is a macro: obs_find_module_file(obs_current_module(), file)
 * so we stub the underlying functions, not the macro itself. */
void rebind_current_settings(struct lens_data *filter) { (void)filter; }
void set_blending_parameters(void) {}

obs_module_t *obs_module_pointer = NULL;

const char *obs_module_text(const char *val) { return val; }

obs_module_t *obs_current_module(void) { return obs_module_pointer; }

char *obs_find_module_file(obs_module_t *module, const char *file) {
  (void)module;
  (void)file;
  char *stub = (char *)bmalloc(8);
  if (stub) stub[0] = '\0';
  return stub;
}
