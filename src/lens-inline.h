#pragma once

// Inline helpers shared by every translation unit in src/. Included from
// lens-data.h so the public surface for .c files is unchanged. Splitting
// these out of lens-data.h keeps the struct-only header lightweight
// (no inline impls bloat every TU that needs only the struct type) and
// makes the boundary between "data declaration" and "implementation"
// explicit.

static inline int clamp_int(int v, int min, int max) {
  return (v < min) ? min : ((v > max) ? max : v);
}

static inline float clamp_float(float v, float min, float max) {
  return (v < min) ? min : ((v > max) ? max : v);
}

static inline bool lens_run_technique_quad(gs_technique_t *tech,
                                           uint32_t width, uint32_t height) {
  if (!tech || width == 0 || height == 0)
    return false;

  size_t passes = gs_technique_begin(tech);
  if (passes == 0)
    return false;

  bool complete = true;
  bool pass_started = false;
  for (size_t i = 0; i < passes; i++) {
    if (!gs_technique_begin_pass(tech, i)) {
      complete = false;
      break;
    }

    pass_started = true;
    lens_draw_quad(width, height);
    gs_technique_end_pass(tech);
  }
  if (pass_started)
    gs_technique_end(tech);
  return complete;
}
