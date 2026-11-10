/**
 * @file obs-utils.c
 * @brief Implementation of the shared OBS helper layer.
 *
 * Reimplements OBS-idiomatic patterns. See obs-utils.h for the
 * contract; see src/lens-core.c for the original (now-replaced)
 * inline code that lived in `capture_input_texture` and the
 * final-composite block in `lens_render`.
 */
#include "obs-utils.h"
#include "lens-data.h"
#include "lens-params.h"
#include <util/dstr.h>
#include <util/platform.h>
#include <string.h>

void setting_visibility(const char *prop_name, bool visible,
                        obs_properties_t *props) {
  if (!prop_name || !props)
    return;
  obs_property_t *p = obs_properties_get(props, prop_name);
  if (!p)
    return;
  obs_property_set_enabled(p, visible);
  obs_property_set_visible(p, visible);
}

void set_blending_parameters(void) {
  gs_blend_state_push();
  gs_reset_blend_state();
  gs_enable_blending(false);
  gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
}

bool get_input_source(struct lens_data *filter, uint32_t width,
                      uint32_t height) {
  if (!filter || !filter->context || !filter->input_texrender || width == 0 ||
      height == 0)
    return false;

  obs_source_t *target = obs_filter_get_target(filter->context);
  obs_source_t *parent = obs_filter_get_parent(filter->context);
  if (!target || !parent)
    return false;

  if (target == filter->context) {
    EMULENS_LOG_WARN("get_input_source: target == context (self-reference) — "
                     "skipping render (target=%p)", (void *)target);
    return false;
  }

  gs_texrender_reset(filter->input_texrender);
  if (!gs_texrender_begin(filter->input_texrender, width, height))
    return false;

  struct vec4 clear_color = {0};
  gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);

  gs_viewport_push();
  gs_set_viewport(0, 0, (int)width, (int)height);
  gs_projection_push();
  gs_ortho(0.0f, (float)width, 0.0f, (float)height, -100.0f, 100.0f);
  gs_matrix_push();
  gs_matrix_identity();

  /*
   * Mirror obs_source_skip_video_filter() using public APIs.
   *
   * For an ordinary synchronous parent source, activate OBS's default Draw
   * effect before invoking its renderer.  Image/text sources depend on that
   * effect for color-matrix parameters and vertex shader state.
   *
   * Custom-draw, async, and upstream-filter targets go through
   * obs_source_video_render().  While a source's filter callback is active,
   * libobs marks the parent as rendering_filter, so this bypasses the current
   * filter rather than recursing back into EmuLens.
   */
  const uint32_t parent_flags = obs_source_get_output_flags(parent);
  const bool parent_custom_draw =
      (parent_flags & OBS_SOURCE_CUSTOM_DRAW) != 0;
  const bool parent_async = (parent_flags & OBS_SOURCE_ASYNC) != 0;

  if (target == parent && !parent_custom_draw && !parent_async)
    obs_source_default_render(target);
  else
    obs_source_video_render(target);

  gs_matrix_pop();
  gs_projection_pop();
  gs_viewport_pop();
  gs_texrender_end(filter->input_texrender);

  return gs_texrender_get_texture(filter->input_texrender) != NULL;
}

void rebind_current_settings(struct lens_data *filter) {
  if (!filter)
    return;

  int mode = (int)atomic_load_explicit(&filter->mode, memory_order_acquire);
  if (mode < 0 || mode >= NUM_MODES || !strategies[mode].update_settings)
    return;

  obs_data_t *settings = NULL;
  pthread_mutex_lock(&filter->settings_mutex);
  if (filter->cached_settings) {
    settings = filter->cached_settings;
    obs_data_addref(settings);
  }
  pthread_mutex_unlock(&filter->settings_mutex);

  if (settings) {
    strategies[mode].update_settings(filter, settings);
    obs_data_release(settings);
  }
}

#define MAX_SHADER_INCLUDE_DEPTH 8

static char *load_shader_recursive(const char *file_path, int depth) {
  if (!file_path)
    return NULL;

  if (depth > MAX_SHADER_INCLUDE_DEPTH) {
    EMULENS_LOG_ERROR("load_shader: max include depth (%d) exceeded at '%s'",
                      MAX_SHADER_INCLUDE_DEPTH, file_path);
    return NULL;
  }

  char *file_str = os_quick_read_utf8_file(file_path);
  if (!file_str)
    return NULL;

  struct dstr output;
  dstr_init(&output);

  char **lines = strlist_split(file_str, '\n', true);
  bfree(file_str);
  if (!lines) {
    dstr_free(&output);
    return NULL;
  }

  for (char **line_ptr = lines; *line_ptr; line_ptr++) {
    char *line = *line_ptr;
    char *p = line;
    bool included = false;

    while (*p == ' ' || *p == '\t')
      p++;

    if (strncmp(p, "#include", 8) == 0) {
      p += 8;
      if (*p == ' ' || *p == '\t' || *p == '"') {
        while (*p == ' ' || *p == '\t')
          p++;
        if (*p == '"') {
          const char *include_start = ++p;
          char *include_end = strchr(p, '"');
          if (include_end) {
            size_t include_len = (size_t)(include_end - include_start);
            char *include_name = bzalloc(include_len + 1);
            if (include_name) {
              memcpy(include_name, include_start, include_len);

              struct dstr include_path;
              dstr_init(&include_path);
              const char *last_slash = strrchr(file_path, '/');
#ifdef _WIN32
              if (!last_slash)
                last_slash = strrchr(file_path, '\\');
#endif
              if (last_slash) {
                size_t dir_len = (size_t)(last_slash - file_path) + 1;
                dstr_ncopy(&include_path, file_path, dir_len);
                dstr_cat(&include_path, include_name);
              } else {
                dstr_copy(&include_path, include_name);
              }

              char *include_content =
                  load_shader_recursive(include_path.array, depth + 1);
              if (include_content) {
                dstr_cat(&output, include_content);
                bfree(include_content);
                included = true;
              }

              dstr_free(&include_path);
              bfree(include_name);
            }
          }
        }
      }
    }

    if (!included) {
      dstr_cat(&output, line);
      dstr_cat(&output, "\n");
    }
  }

  strlist_free(lines);
  return output.array;
}

char *load_shader_from_file(const char *file_path) {
  return load_shader_recursive(file_path, 0);
}
