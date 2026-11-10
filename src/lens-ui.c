#include "lens-data.h"
#include "lens-params.h"
#include "obs-utils.h"
#include <obs-module.h>

// -------------------------------------------------------------------------------------------------
// Visibility callbacks (unchanged — gate sub-params behind bool toggles)
// -------------------------------------------------------------------------------------------------
static bool effect_mode_modified(obs_properties_t *props, obs_property_t *p,
                                 obs_data_t *settings);
static bool handheld_blur_enable_modified(obs_properties_t *props,
                                          obs_property_t *p,
                                          obs_data_t *settings);
static bool sb_colorize_modified(obs_properties_t *props, obs_property_t *p,
                                 obs_data_t *settings);
static bool leak_color_shift_modified(obs_properties_t *props,
                                      obs_property_t *p, obs_data_t *settings);
static bool part_source_color_modified(obs_properties_t *props,
                                       obs_property_t *p, obs_data_t *settings);
static bool part_source_brightness_modified(obs_properties_t *props,
                                            obs_property_t *p,
                                            obs_data_t *settings);
static bool part_use_poly_modified(obs_properties_t *props, obs_property_t *p,
                                   obs_data_t *settings);
static bool bokeh_high_response_modified(obs_properties_t *props,
                                         obs_property_t *p,
                                         obs_data_t *settings);

// -------------------------------------------------------------------------------------------------
// Section header helpers
// -------------------------------------------------------------------------------------------------
static void section_header(obs_properties_t *props, const char *label) {
  static int sec_counter = 0;
  char name[48];
  snprintf(name, sizeof(name), "section_%d", sec_counter++);
  obs_properties_add_text(props, name, label, OBS_TEXT_INFO);
}

// Forward declarations for all per-mode property builders (used via X-macro)
#define X(id, suffix, display_name, tech)                                      \
  static void add_##suffix##_properties(obs_properties_t *props);
EMULENS_MODES
#undef X

static obs_data_t *load_preset_data(const char *preset_name) {
  if (!preset_name || !*preset_name)
    return NULL;

  // Preset names come from persisted settings, not only the combo box.
  // Restrict them to a single shipped filename component.
  if (strstr(preset_name, "..") || strchr(preset_name, '/') ||
      strchr(preset_name, '\\')) {
    EMULENS_LOG_WARN("Ignoring invalid preset name: %s", preset_name);
    return NULL;
  }

  struct dstr rel_path = {0};
  dstr_printf(&rel_path, "presets/%s.json", preset_name);

  char *full_path = obs_module_file(rel_path.array);
  dstr_free(&rel_path);

  if (!full_path) {
    EMULENS_LOG_ERROR("Could not locate preset file: presets/%s.json",
                      preset_name);
    return NULL;
  }

  obs_data_t *preset_data = NULL;
  char *json_data = os_quick_read_utf8_file(full_path);

  if (json_data) {
    preset_data = obs_data_create_from_json(json_data);
    if (!preset_data)
      EMULENS_LOG_ERROR("Failed to parse JSON from preset: %s", preset_name);
    bfree(json_data);
  } else {
    EMULENS_LOG_ERROR("Failed to read preset file: %s", full_path);
  }

  bfree(full_path);
  return preset_data;
}

// -------------------------------------------------------------------------------------------------
// Bool-gated visibility callbacks
//
// Each callback reads a gate bool and toggles a set of dependent
// properties via setting_visibility(). The helper sets both
// obs_property_set_visible and obs_property_set_enabled so a
// hidden sub-property cannot be edited programmatically.
// -------------------------------------------------------------------------------------------------

static bool handheld_blur_enable_modified(obs_properties_t *props,
                                          obs_property_t *p,
                                          obs_data_t *settings) {
  UNUSED_PARAMETER(p);
  bool enabled = obs_data_get_bool(settings, P_HAND_BLUR_ENABLE);
  setting_visibility(P_HAND_BLUR_AMP, enabled, props);
  setting_visibility(P_HAND_BLUR_SPEED, enabled, props);
  setting_visibility(P_HAND_BLUR_STATIC, enabled, props);
  setting_visibility(P_HAND_FEATHER, enabled, props);
  return true;
}

static bool sb_colorize_modified(obs_properties_t *props, obs_property_t *p,
                                 obs_data_t *settings) {
  UNUSED_PARAMETER(p);
  bool colorize = obs_data_get_bool(settings, P_SB_COLORIZE);
  setting_visibility(P_SB_COLOR, colorize, props);
  return true;
}

static bool leak_color_shift_modified(obs_properties_t *props,
                                      obs_property_t *p, obs_data_t *settings) {
  UNUSED_PARAMETER(p);
  bool enabled = obs_data_get_bool(settings, P_LEAK_COLOR_ENABLE_SHIFT);
  setting_visibility(P_LEAK_COLOR_SHIFT, enabled, props);
  setting_visibility(P_LEAK_COLOR_SPEED, enabled, props);
  return true;
}

static bool part_source_color_modified(obs_properties_t *props,
                                       obs_property_t *p,
                                       obs_data_t *settings) {
  UNUSED_PARAMETER(p);
  bool enabled = obs_data_get_bool(settings, P_PART_SOURCE_COLOR);
  setting_visibility(P_PART_SOURCE_INFLUENCE, enabled, props);
  return true;
}

static bool part_source_brightness_modified(obs_properties_t *props,
                                            obs_property_t *p,
                                            obs_data_t *settings) {
  UNUSED_PARAMETER(p);
  bool enabled = obs_data_get_bool(settings, P_PART_SOURCE_BRIGHTNESS);
  setting_visibility(P_PART_SOURCE_THRESHOLD, enabled, props);
  setting_visibility(P_PART_SOURCE_STRENGTH, enabled, props);
  return true;
}

static bool part_use_poly_modified(obs_properties_t *props, obs_property_t *p,
                                   obs_data_t *settings) {
  UNUSED_PARAMETER(p);
  bool enabled = obs_data_get_bool(settings, P_PART_USE_POLY);
  setting_visibility(P_PART_POLY_SIDES, enabled, props);
  setting_visibility(P_PART_POLY_ROT, enabled, props);
  setting_visibility(P_PART_POLY_ROT_SPEED, enabled, props);
  return true;
}

static bool bokeh_high_response_modified(obs_properties_t *props,
                                         obs_property_t *p,
                                         obs_data_t *settings) {
  UNUSED_PARAMETER(p);
  bool enabled = obs_data_get_bool(settings, P_BOKEH_HIGH_RESPONSE);
  setting_visibility(P_BOKEH_THRESHOLD, enabled, props);
  return true;
}

static void refresh_bool_visibility(obs_properties_t *props,
                                    obs_data_t *settings) {
  handheld_blur_enable_modified(props, NULL, settings);
  sb_colorize_modified(props, NULL, settings);
  leak_color_shift_modified(props, NULL, settings);
  part_source_color_modified(props, NULL, settings);
  part_source_brightness_modified(props, NULL, settings);
  part_use_poly_modified(props, NULL, settings);
  bokeh_high_response_modified(props, NULL, settings);
}

// -------------------------------------------------------------------------------------------------
// Callbacks
// -------------------------------------------------------------------------------------------------

static bool load_preset_callback(obs_properties_t *props, obs_property_t *p,
                                 obs_data_t *settings) {
  UNUSED_PARAMETER(p);
  const char *preset_name = obs_data_get_string(settings, UI_PRESET_LIST);
  if (preset_name && *preset_name) {
    obs_data_t *preset_data = load_preset_data(preset_name);
    if (preset_data) {
      obs_data_apply(settings, preset_data);
      obs_data_release(preset_data);
      effect_mode_modified(props, NULL, settings);
      refresh_bool_visibility(props, settings);
    }
  }
  obs_data_set_string(settings, UI_PRESET_LIST, "");
  return true;
}

// -------------------------------------------------------------------------------------------------
// Forward declarations
// -------------------------------------------------------------------------------------------------

static void add_preset_properties(obs_properties_t *props);
static void populate_preset_entries(obs_property_t *presets, int mode_index);
static const char *mode_group_name(enum emulens_mode mode);
static enum emulens_mode mode_from_settings(obs_data_t *settings);

// -------------------------------------------------------------------------------------------------
// Mode helpers
// -------------------------------------------------------------------------------------------------

static const char *mode_group_name(enum emulens_mode mode) {
  static const char *group_names[NUM_MODES] = {
#define X(id, suffix, display_name, tech) "group_" #suffix,
      EMULENS_MODES
#undef X
  };
  if (mode < 0 || mode >= NUM_MODES)
    return "group_vidicon";
  return group_names[mode];
}

static enum emulens_mode mode_from_settings(obs_data_t *settings) {
  int selected_mode = MODE_VIDICON;
  if (settings)
    selected_mode = (int)obs_data_get_int(settings, UI_MODE);
  if (selected_mode < 0 || selected_mode >= NUM_MODES)
    selected_mode = MODE_VIDICON;
  return (enum emulens_mode)selected_mode;
}

static bool effect_mode_modified(obs_properties_t *props, obs_property_t *p,
                                 obs_data_t *settings) {
  UNUSED_PARAMETER(p);
  enum emulens_mode selected_mode = mode_from_settings(settings);

  for (int i = 0; i < NUM_MODES; i++) {
    obs_property_t *group = obs_properties_get(props, mode_group_name(i));
    if (group)
      obs_property_set_visible(group, i == (int)selected_mode);
  }

  obs_property_t *presets_prop = obs_properties_get(props, UI_PRESET_LIST);
  if (presets_prop) {
    obs_property_list_clear(presets_prop);
    populate_preset_entries(presets_prop, (int)selected_mode);
  }

  return true;
}

// -------------------------------------------------------------------------------------------------
// Main Properties Builder
// -------------------------------------------------------------------------------------------------

obs_properties_t *emulens_properties(void *data) {
  obs_properties_t *props = obs_properties_create();
  if (!props)
    return NULL;
  obs_properties_set_param(props, data, NULL);

  // Mode selector
  obs_property_t *mode_prop =
      obs_properties_add_list(props, UI_MODE, obs_module_text("EffectType"),
                              OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
  if (!mode_prop) {
    obs_properties_destroy(props);
    return NULL;
  }
#define X(id, suffix, display_name, tech)                                      \
  obs_property_list_add_int(mode_prop, obs_module_text(display_name),          \
                            MODE_##id);
  EMULENS_MODES
#undef X
  obs_property_set_modified_callback(mode_prop, effect_mode_modified);

  // Preset selector — right below mode so users can quick-start
  add_preset_properties(props);

  // Per-mode groups
#define X(id, suffix, display_name, tech)                                      \
  do {                                                                         \
    obs_properties_t *group_props = obs_properties_create();                   \
    if (!group_props)                                                           \
      break;                                                                    \
    add_##suffix##_properties(group_props);                                    \
    obs_property_t *group = obs_properties_add_group(                          \
        props, "group_" #suffix, display_name, OBS_GROUP_NORMAL, group_props); \
    if (group)                                                                 \
      obs_property_set_visible(group, false);                                  \
  } while (0);
  EMULENS_MODES
#undef X

  // Initialize visibility and presets for active mode
  enum emulens_mode selected_mode = MODE_VIDICON;
  struct lens_data *filter = data;
  obs_data_t *init_settings = NULL;
  if (filter && filter->context) {
    init_settings = obs_source_get_settings(filter->context);
    if (init_settings)
      selected_mode = mode_from_settings(init_settings);
  }

  obs_property_t *selected_group =
      obs_properties_get(props, mode_group_name(selected_mode));
  if (selected_group)
    obs_property_set_visible(selected_group, true);

  obs_property_t *presets_prop = obs_properties_get(props, UI_PRESET_LIST);
  if (presets_prop) {
    obs_property_list_clear(presets_prop);
    populate_preset_entries(presets_prop, (int)selected_mode);
  }

  if (init_settings) {
    refresh_bool_visibility(props, init_settings);
    obs_data_release(init_settings);
  }

  return props;
}

void emulens_get_defaults(obs_data_t *settings) {
  obs_data_set_default_int(settings, UI_MODE, MODE_VIDICON);
  obs_data_set_default_string(settings, UI_PRESET_LIST, "");
  for (int i = 0; i < NUM_MODES; i++) {
    if (strategies[i].get_defaults)
      strategies[i].get_defaults(settings);
  }
}

// -------------------------------------------------------------------------------------------------
// Vidicon
// -------------------------------------------------------------------------------------------------

static void add_vidicon_properties(obs_properties_t *props) {
  obs_property_t *p;

  section_header(props, "Trails");
  p = obs_properties_add_float_slider(
      props, P_VID_DECAY, obs_module_text("TrailDecay"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("TrailDecayTooltip"));
  p = obs_properties_add_float_slider(props, P_VID_LAG_NONLINEAR,
                                      obs_module_text("LagNonlinear"), 0.0, 1.0,
                                      0.01);
  obs_property_set_long_description(p, obs_module_text("LagNonlinearTooltip"));
  p = obs_properties_add_bool(props, P_VID_COMET, obs_module_text("CometTail"));
  obs_property_set_long_description(p, obs_module_text("CometTailTooltip"));
  p = obs_properties_add_float_slider(props, P_VID_COMET_LENGTH,
                                      obs_module_text("CometLength"), 1.0, 20.0,
                                      0.5);
  obs_property_set_long_description(p, obs_module_text("CometLengthTooltip"));
  p = obs_properties_add_float_slider(
      props, P_VID_BEAM_PULL, obs_module_text("BeamPull"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("BeamPullTooltip"));

  section_header(props, "Threshold");
  p = obs_properties_add_float_slider(
      props, P_VID_THRESHOLD, obs_module_text("LumaThreshold"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("LumaThresholdTooltip"));

  section_header(props, "Color");
  p = obs_properties_add_color_alpha(props, P_VID_COLOR,
                                     obs_module_text("TrailColor"));
  obs_property_set_long_description(p, obs_module_text("TrailColorTooltip"));
  p = obs_properties_add_float_slider(
      props, P_VID_BLEND, obs_module_text("ColorMix"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("ColorMixTooltip"));
  p = obs_properties_add_bool(props, P_VID_SOURCE_COLOR,
                              obs_module_text("SourceColor"));
  obs_property_set_long_description(p, obs_module_text("SourceColorTooltip"));

  section_header(props, "Channel Lag");
  p = obs_properties_add_float_slider(
      props, P_VID_RED_LAG, obs_module_text("RedLag"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("RedLagTooltip"));
  p = obs_properties_add_float_slider(
      props, P_VID_BLUE_LAG, obs_module_text("BlueLag"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("BlueLagTooltip"));

  section_header(props, "Tube Artifacts");
  p = obs_properties_add_float_slider(
      props, P_VID_SCANLINES, obs_module_text("ScanLines"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("ScanLinesTooltip"));
  p = obs_properties_add_float_slider(
      props, P_VID_NOISE, obs_module_text("TubeNoise"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("TubeNoiseTooltip"));
  p = obs_properties_add_float_slider(
      props, P_VID_HALATION, obs_module_text("Halation"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("HalationTooltip"));
  p = obs_properties_add_float_slider(
      props, P_VID_GAMMA, obs_module_text("VidGamma"), 0.3, 2.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("VidGammaTooltip"));
  p = obs_properties_add_float_slider(
      props, P_VID_HUM, obs_module_text("HumBar"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("HumBarTooltip"));

  section_header(props, "Registration");
  p = obs_properties_add_float_slider(
      props, P_VID_REGISTER, obs_module_text("RegisterOffset"), 0.0, 10.0, 0.1);
  obs_property_set_long_description(p,
                                    obs_module_text("RegisterOffsetTooltip"));
  p = obs_properties_add_float_slider(props, P_VID_REGISTER_DRIFT,
                                      obs_module_text("RegisterDrift"), 0.0,
                                      5.0, 0.1);
  obs_property_set_long_description(p, obs_module_text("RegisterDriftTooltip"));
}

// -------------------------------------------------------------------------------------------------
// Starburst
// -------------------------------------------------------------------------------------------------

static void add_starburst_properties(obs_properties_t *props) {
  obs_property_t *p;

  section_header(props, "Core");
  p = obs_properties_add_float_slider(props, P_SB_THRESHOLD,
                                      obs_module_text("Threshold"),
                                      SB_THRESHOLD_MIN, SB_THRESHOLD_MAX, 0.01);
  obs_property_set_long_description(p, obs_module_text("ThresholdTooltip"));
  p = obs_properties_add_float_slider(props, P_SB_INTENSITY,
                                      obs_module_text("Intensity"),
                                      SB_INTENSITY_MIN, SB_INTENSITY_MAX, 0.1);
  obs_property_set_long_description(p, obs_module_text("IntensityTooltip"));
  p = obs_properties_add_int_slider(props, P_SB_POINTS,
                                    obs_module_text("Points"), 2, 16, 1);
  obs_property_set_long_description(p, obs_module_text("PointsTooltip"));

  section_header(props, "Ray Shape");
  p = obs_properties_add_float_slider(
      props, P_SB_RAY_LENGTH, obs_module_text("RayLength"), 0.01, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("RayLengthTooltip"));
  p = obs_properties_add_float_slider(
      props, P_SB_THICKNESS, obs_module_text("Thickness"), 0.1, 10.0, 0.1);
  obs_property_set_long_description(p, obs_module_text("ThicknessTooltip"));
  p = obs_properties_add_float_slider(
      props, P_SB_SMOOTHNESS, obs_module_text("Smoothness"), 0.1, 10.0, 0.1);
  obs_property_set_long_description(p, obs_module_text("SmoothnessTooltip"));
  p = obs_properties_add_bool(props, P_SB_ANAMORPHIC,
                              obs_module_text("Anamorphic"));
  obs_property_set_long_description(p, obs_module_text("AnamorphicTooltip"));

  section_header(props, "Color");
  p = obs_properties_add_bool(props, P_SB_COLORIZE,
                              obs_module_text("ColorizeRays"));
  obs_property_set_long_description(p, obs_module_text("ColorizeRaysTooltip"));
  obs_property_set_modified_callback(p, sb_colorize_modified);
  p = obs_properties_add_color_alpha(props, P_SB_COLOR,
                                     obs_module_text("RayColor"));
  obs_property_set_long_description(p, obs_module_text("RayColorTooltip"));
  p = obs_properties_add_float_slider(props, P_SB_CHROMA,
                                      obs_module_text("ChromaticAberration"),
                                      0.0, 2.0, 0.05);
  obs_property_set_long_description(
      p, obs_module_text("ChromaticAberrationTooltip"));

  section_header(props, "Animation");
  p = obs_properties_add_float_slider(
      props, P_SB_ROTATION, obs_module_text("Rotation"), 0.0, 360.0, 1.0);
  obs_property_set_long_description(p, obs_module_text("RotationTooltip"));
  p = obs_properties_add_float_slider(props, P_SB_ROT_SPEED,
                                       obs_module_text("RotationSpeed"), -3.0,
                                       3.0, 0.1);
  obs_property_set_long_description(p, obs_module_text("RotationSpeedTooltip"));
  p = obs_properties_add_float_slider(
      props, P_SB_PULSE_SPEED, obs_module_text("PulseSpeed"), 0.0, 3.0, 0.1);
  obs_property_set_long_description(p, obs_module_text("PulseSpeedTooltip"));
  p = obs_properties_add_float_slider(
      props, P_SB_PULSE_AMP, obs_module_text("PulseAmount"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("PulseAmountTooltip"));

  section_header(props, "Quality");
  p = obs_properties_add_int_slider(props, P_SB_SAMPLES,
                                    obs_module_text("Samples"), 4, 16, 1);
  obs_property_set_long_description(p, obs_module_text("SamplesTooltip"));

  section_header(props, "Blending");
  obs_property_t *sb_blend = obs_properties_add_list(
      props, P_SB_BLEND_MODE, obs_module_text("BlendMode"), OBS_COMBO_TYPE_LIST,
      OBS_COMBO_FORMAT_INT);
  obs_property_set_long_description(sb_blend,
                                    obs_module_text("BlendModeTooltip"));
  obs_property_list_add_int(sb_blend, obs_module_text("Additive"), 0);
  obs_property_list_add_int(sb_blend, obs_module_text("Screen"), 1);
  obs_property_list_add_int(sb_blend, obs_module_text("Overlay"), 2);
}

// -------------------------------------------------------------------------------------------------
// Leak
// -------------------------------------------------------------------------------------------------

static void add_leak_properties(obs_properties_t *props) {
  obs_property_t *p;

  section_header(props, "Core");
  p = obs_properties_add_float_slider(
      props, P_LEAK_INTENSITY, obs_module_text("Intensity"), 0.0, 5.0, 0.05);
  obs_property_set_long_description(p, obs_module_text("IntensityTooltip"));
  p = obs_properties_add_float_slider(
      props, P_LEAK_SCALE, obs_module_text("NoiseScale"), 0.1, 20.0, 0.1);
  obs_property_set_long_description(p, obs_module_text("NoiseScaleTooltip"));
  p = obs_properties_add_float_slider(
      props, P_LEAK_SPEED, obs_module_text("AnimationSpeed"), 0.0, 2.0, 0.1);
  obs_property_set_long_description(p,
                                    obs_module_text("AnimationSpeedTooltip"));

  section_header(props, "Color");
  p = obs_properties_add_color_alpha(props, P_LEAK_COLOR,
                                     obs_module_text("PrimaryColor"));
  obs_property_set_long_description(p, obs_module_text("PrimaryColorTooltip"));
  p = obs_properties_add_bool(props, P_LEAK_COLOR_ENABLE_SHIFT,
                              obs_module_text("EnableColorShift"));
  obs_property_set_long_description(p,
                                    obs_module_text("EnableColorShiftTooltip"));
  obs_property_set_modified_callback(p, leak_color_shift_modified);
  p = obs_properties_add_color_alpha(props, P_LEAK_COLOR_SHIFT,
                                     obs_module_text("SecondaryColor"));
  obs_property_set_long_description(p,
                                    obs_module_text("SecondaryColorTooltip"));
   p = obs_properties_add_float_slider(props, P_LEAK_COLOR_SPEED,
                                       obs_module_text("ColorShiftSpeed"), 0.0,
                                       2.0, 0.1);
  obs_property_set_long_description(p,
                                    obs_module_text("ColorShiftSpeedTooltip"));

  section_header(props, "Edge Bias");
  p = obs_properties_add_float_slider(
      props, P_LEAK_BIAS_TOP, obs_module_text("BiasTop"), 0.0, 5.0, 0.05);
  obs_property_set_long_description(p, obs_module_text("BiasTopTooltip"));
  p = obs_properties_add_float_slider(
      props, P_LEAK_BIAS_BOT, obs_module_text("BiasBottom"), 0.0, 5.0, 0.05);
  obs_property_set_long_description(p, obs_module_text("BiasBottomTooltip"));
  p = obs_properties_add_float_slider(
      props, P_LEAK_BIAS_LEFT, obs_module_text("BiasLeft"), 0.0, 5.0, 0.05);
  obs_property_set_long_description(p, obs_module_text("BiasLeftTooltip"));
  p = obs_properties_add_float_slider(
      props, P_LEAK_BIAS_RIGHT, obs_module_text("BiasRight"), 0.0, 5.0, 0.05);
  obs_property_set_long_description(p, obs_module_text("BiasRightTooltip"));
  p = obs_properties_add_float_slider(props, P_LEAK_EDGE_FALLOFF,
                                      obs_module_text("EdgeFalloff"), 0.1, 20.0,
                                      0.1);
  obs_property_set_long_description(p, obs_module_text("EdgeFalloffTooltip"));

  section_header(props, "Pattern");
  p = obs_properties_add_float_slider(props, P_LEAK_STREAKINESS,
                                      obs_module_text("Streakiness"), 0.1, 20.0,
                                      0.5);
  obs_property_set_long_description(p, obs_module_text("StreakinessTooltip"));
  p = obs_properties_add_float_slider(
      props, P_LEAK_CONTRAST, obs_module_text("Contrast"), 0.1, 10.0, 0.05);
  obs_property_set_long_description(p, obs_module_text("ContrastTooltip"));
  p = obs_properties_add_int_slider(
      props, P_LEAK_COMPLEXITY, obs_module_text("NoiseComplexity"), 1, 10, 1);
  obs_property_set_long_description(p,
                                    obs_module_text("NoiseComplexityTooltip"));

  section_header(props, "Blending");
  obs_property_t *blend_list = obs_properties_add_list(
      props, P_LEAK_BLEND_MODE, obs_module_text("BlendMode"),
      OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
  obs_property_set_long_description(blend_list,
                                    obs_module_text("BlendModeTooltip"));
  obs_property_list_add_int(blend_list, obs_module_text("AlphaBlend"), 0);
  obs_property_list_add_int(blend_list, obs_module_text("LeakLighten"), 1);
  obs_property_list_add_int(blend_list, obs_module_text("Film"), 2);
  obs_property_list_add_int(blend_list, obs_module_text("Overlay"), 3);
  obs_property_list_add_int(blend_list, obs_module_text("SoftLight"), 4);
}

// -------------------------------------------------------------------------------------------------
// Handheld
// -------------------------------------------------------------------------------------------------

static void add_handheld_properties(obs_properties_t *props) {
  obs_property_t *p;

  section_header(props, "Position Shake");
  p = obs_properties_add_float_slider(
      props, P_HAND_POS_AMP, obs_module_text("PosShakeAmt"), HAND_POS_AMP_MIN,
      HAND_POS_AMP_MAX, 0.001);
  obs_property_set_long_description(p, obs_module_text("PosShakeAmtTooltip"));
  p = obs_properties_add_float_slider(
      props, P_HAND_POS_SPEED, obs_module_text("PosShakeSpd"),
      HAND_POS_SPEED_MIN, HAND_POS_SPEED_MAX, 0.1);
  obs_property_set_long_description(p, obs_module_text("PosShakeSpdTooltip"));

  section_header(props, "Rotation");
  p = obs_properties_add_float_slider(props, P_HAND_ROT_AMP,
                                      obs_module_text("RotAmount"),
                                      HAND_ROT_AMP_MIN, HAND_ROT_AMP_MAX, 0.1);
  obs_property_set_long_description(p, obs_module_text("RotAmountTooltip"));
  p = obs_properties_add_float_slider(
      props, P_HAND_ROT_SPEED, obs_module_text("RotationSpeed"), 0.0, 5.0, 0.1);
  obs_property_set_long_description(p, obs_module_text("RotationSpeedTooltip"));

  section_header(props, "Zoom Wobble");
  p = obs_properties_add_float_slider(
      props, P_HAND_ZOOM_AMP, obs_module_text("ZoomFluctAmt"), 0.0, 0.5, 0.001);
  obs_property_set_long_description(p, obs_module_text("ZoomFluctAmtTooltip"));
  p = obs_properties_add_float_slider(
      props, P_HAND_ZOOM_SPEED, obs_module_text("ZoomFluctSpd"), 0.0, 5.0, 0.1);
  obs_property_set_long_description(p, obs_module_text("ZoomFluctSpdTooltip"));

  section_header(props, "Human Motion");
  p = obs_properties_add_float_slider(
      props, P_HAND_TREMOR_AMP, obs_module_text("TremorAmount"),
      HAND_TREMOR_AMP_MIN, HAND_TREMOR_AMP_MAX, 0.01);
  obs_property_set_long_description(p, obs_module_text("TremorAmountTooltip"));
  p = obs_properties_add_float_slider(
      props, P_HAND_BREATH_AMP, obs_module_text("BreathingAmount"),
      HAND_BREATH_AMP_MIN, HAND_BREATH_AMP_MAX, 0.01);
  obs_property_set_long_description(p,
                                    obs_module_text("BreathingAmountTooltip"));
  p = obs_properties_add_float_slider(
      props, P_HAND_STEP_AMP, obs_module_text("StepBobAmount"),
      HAND_STEP_AMP_MIN, HAND_STEP_AMP_MAX, 0.01);
  obs_property_set_long_description(p, obs_module_text("StepBobAmountTooltip"));
  p = obs_properties_add_float_slider(
      props, P_HAND_STEP_SPEED, obs_module_text("StepBobSpeed"),
      HAND_STEP_SPEED_MIN, HAND_STEP_SPEED_MAX, 0.1);
  obs_property_set_long_description(p, obs_module_text("StepBobSpeedTooltip"));
  p = obs_properties_add_float_slider(
      props, P_HAND_CORRECTION_AMP, obs_module_text("CorrectionAmount"),
      HAND_CORRECTION_AMP_MIN, HAND_CORRECTION_AMP_MAX, 0.01);
  obs_property_set_long_description(p,
                                    obs_module_text("CorrectionAmountTooltip"));
  p = obs_properties_add_float_slider(props, P_HAND_INERTIA,
                                      obs_module_text("InertiaAmount"),
                                      HAND_INERTIA_MIN, HAND_INERTIA_MAX, 0.01);
  obs_property_set_long_description(p, obs_module_text("InertiaAmountTooltip"));
  p = obs_properties_add_float_slider(
      props, P_HAND_BLUR_RESPONSE, obs_module_text("BlurResponse"),
      HAND_BLUR_RESPONSE_MIN, HAND_BLUR_RESPONSE_MAX, 0.1);
  obs_property_set_long_description(p, obs_module_text("BlurResponseTooltip"));

  section_header(props, "Motion Blur");
  p = obs_properties_add_bool(props, P_HAND_BLUR_ENABLE,
                              obs_module_text("EnableBlur"));
  obs_property_set_long_description(p, obs_module_text("EnableBlurTooltip"));
  obs_property_set_modified_callback(p, handheld_blur_enable_modified);
  p = obs_properties_add_float_slider(
      props, P_HAND_BLUR_AMP, obs_module_text("MaxBlurAmt"), 0.0, 20.0, 0.1);
  obs_property_set_long_description(p, obs_module_text("MaxBlurAmtTooltip"));
  p = obs_properties_add_float_slider(props, P_HAND_BLUR_SPEED,
                                      obs_module_text("BlurPulsationSpd"), 0.0,
                                      10.0, 0.1);
  obs_property_set_long_description(p,
                                    obs_module_text("BlurPulsationSpdTooltip"));
  p = obs_properties_add_float_slider(props, P_HAND_BLUR_STATIC,
                                      obs_module_text("StaticBlurRadius"), 0.0,
                                      20.0, 0.1);
  obs_property_set_long_description(p,
                                    obs_module_text("StaticBlurRadiusTooltip"));

  section_header(props, "Edge Treatment");
  p = obs_properties_add_float_slider(
      props, P_HAND_FEATHER, obs_module_text("FocusFeather"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("FocusFeatherTooltip"));
}

// -------------------------------------------------------------------------------------------------
// Bokeh
// -------------------------------------------------------------------------------------------------

static void add_bokeh_properties(obs_properties_t *props) {
  obs_property_t *p;

  section_header(props, "Core");
  p = obs_properties_add_float_slider(
      props, P_BOKEH_RADIUS, obs_module_text("MaxBlurRadius"), 0.0, 50.0, 0.5);
  obs_property_set_long_description(p, obs_module_text("MaxBlurRadiusTooltip"));
  p = obs_properties_add_float_slider(
      props, P_BOKEH_POWER, obs_module_text("BlurCurve"), 0.1, 10.0, 0.1);
  obs_property_set_long_description(p, obs_module_text("BlurCurveTooltip"));
  p = obs_properties_add_int_slider(props, P_BOKEH_SHAPE,
                                    obs_module_text("ApertureShape"), 0, 8, 1);
  obs_property_set_long_description(p, obs_module_text("ApertureShapeTooltip"));

  section_header(props, "Focus Area");
  p = obs_properties_add_float_slider(
      props, P_BOKEH_FOCUS_X, obs_module_text("FocusX"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("FocusXTooltip"));
  p = obs_properties_add_float_slider(
      props, P_BOKEH_FOCUS_Y, obs_module_text("FocusY"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("FocusYTooltip"));
  p = obs_properties_add_float_slider(
      props, P_BOKEH_WIDTH, obs_module_text("FocusWidth"), 0.01, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("FocusWidthTooltip"));
  p = obs_properties_add_float_slider(
      props, P_BOKEH_FALLOFF, obs_module_text("EdgeFalloff"), 0.001, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("EdgeFalloffTooltip"));

  section_header(props, "Highlights");
  p = obs_properties_add_float_slider(
      props, P_BOKEH_BOOST, obs_module_text("HighlightBoost"), 0.0, 50.0, 0.5);
  obs_property_set_long_description(p,
                                    obs_module_text("HighlightBoostTooltip"));
  p = obs_properties_add_bool(props, P_BOKEH_HIGH_RESPONSE,
                              obs_module_text("HighResponse"));
  obs_property_set_long_description(p, obs_module_text("HighResponseTooltip"));
  obs_property_set_modified_callback(p, bokeh_high_response_modified);
  p = obs_properties_add_float_slider(props, P_BOKEH_THRESHOLD,
                                      obs_module_text("HighlightThreshold"),
                                      0.0, 1.0, 0.01);
  obs_property_set_long_description(
      p, obs_module_text("HighlightThresholdTooltip"));
}

// -------------------------------------------------------------------------------------------------
// Particles
// -------------------------------------------------------------------------------------------------

static void add_particles_properties(obs_properties_t *props) {
  obs_property_t *p;

  section_header(props, "Core");
  p = obs_properties_add_float_slider(props, P_PART_DENSITY,
                                      obs_module_text("ParticleDensity"), 1.0,
                                      40.0, 0.5);
  obs_property_set_long_description(p,
                                    obs_module_text("ParticleDensityTooltip"));
  p = obs_properties_add_float_slider(
      props, P_PART_SIZE, obs_module_text("BaseSize"), 0.001, 0.1, 0.001);
  obs_property_set_long_description(p, obs_module_text("BaseSizeTooltip"));
  p = obs_properties_add_float_slider(
      props, P_PART_SIZE_VAR, obs_module_text("SizeVariation"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("SizeVariationTooltip"));
  p = obs_properties_add_float_slider(
      props, P_PART_SPEED, obs_module_text("ParticleSpeed"), 0.0, 5.0, 0.05);
  obs_property_set_long_description(p, obs_module_text("ParticleSpeedTooltip"));

  section_header(props, "Wind & Wobble");
  p = obs_properties_add_float_slider(
      props, P_PART_WIND_X, obs_module_text("WindX"), -1.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("WindXTooltip"));
  p = obs_properties_add_float_slider(
      props, P_PART_WIND_Y, obs_module_text("WindY"), -1.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("WindYTooltip"));
  p = obs_properties_add_float_slider(
      props, P_PART_WOBBLE, obs_module_text("WobbleAmt"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("WobbleAmtTooltip"));

  section_header(props, "Color");
  p = obs_properties_add_color_alpha(props, P_PART_COLOR_START,
                                     obs_module_text("ColorStart"));
  obs_property_set_long_description(p, obs_module_text("ColorStartTooltip"));
  p = obs_properties_add_color_alpha(props, P_PART_COLOR_END,
                                     obs_module_text("ColorEnd"));
  obs_property_set_long_description(p, obs_module_text("ColorEndTooltip"));
  p = obs_properties_add_bool(props, P_PART_SOURCE_COLOR,
                              obs_module_text("InheritSourceColor"));
  obs_property_set_long_description(
      p, obs_module_text("InheritSourceColorTooltip"));
  obs_property_set_modified_callback(p, part_source_color_modified);
  p = obs_properties_add_float_slider(props, P_PART_SOURCE_INFLUENCE,
                                      obs_module_text("ColorInheritance"), 0.0,
                                      1.0, 0.01);
  obs_property_set_long_description(p,
                                    obs_module_text("ColorInheritanceTooltip"));

  section_header(props, "Source-Driven Spawning");
  p = obs_properties_add_bool(props, P_PART_SOURCE_BRIGHTNESS,
                              obs_module_text("SpawnOnHighlights"));
  obs_property_set_long_description(
      p, obs_module_text("SpawnOnHighlightsTooltip"));
  obs_property_set_modified_callback(p, part_source_brightness_modified);
  p = obs_properties_add_float_slider(props, P_PART_SOURCE_THRESHOLD,
                                      obs_module_text("SpawnThreshold"), 0.0,
                                      1.0, 0.01);
  obs_property_set_long_description(p,
                                    obs_module_text("SpawnThresholdTooltip"));
  p = obs_properties_add_float_slider(props, P_PART_SOURCE_STRENGTH,
                                      obs_module_text("SpawnDensity"), 0.0,
                                      5.0, 0.1);
  obs_property_set_long_description(p, obs_module_text("SpawnDensityTooltip"));

  section_header(props, "Focus Area");
  p = obs_properties_add_float_slider(
      props, P_PART_FOCUS_X, obs_module_text("FocusCenter_X"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("FocusCenterXTooltip"));
  p = obs_properties_add_float_slider(
      props, P_PART_FOCUS_Y, obs_module_text("FocusCenter_Y"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("FocusCenterYTooltip"));
  p = obs_properties_add_float_slider(props, P_PART_FOCUS_STRENGTH,
                                      obs_module_text("DepthConcentration"),
                                      0.0, 1.0, 0.01);
  obs_property_set_long_description(
      p, obs_module_text("DepthConcentrationTooltip"));
  p = obs_properties_add_float_slider(props, P_PART_BLUR_EDGE,
                                      obs_module_text("PerimeterBlur"), 0.0,
                                      1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("PerimeterBlurTooltip"));

  section_header(props, "Advanced");
  p = obs_properties_add_bool(props, P_PART_USE_POLY,
                              obs_module_text("EnablePolygonMode"));
  obs_property_set_long_description(
      p, obs_module_text("EnablePolygonModeTooltip"));
  obs_property_set_modified_callback(p, part_use_poly_modified);
  p = obs_properties_add_int_slider(props, P_PART_POLY_SIDES,
                                    obs_module_text("Sides"), 3, 10, 1);
  obs_property_set_long_description(p, obs_module_text("SidesTooltip"));
  p = obs_properties_add_float_slider(
      props, P_PART_POLY_ROT, obs_module_text("BaseRotation"), 0.0, 360.0, 1.0);
  obs_property_set_long_description(p, obs_module_text("BaseRotationTooltip"));
  p = obs_properties_add_float_slider(props, P_PART_POLY_ROT_SPEED,
                                      obs_module_text("SpinSpeed"), -10.0, 10.0,
                                      0.1);
  obs_property_set_long_description(p, obs_module_text("SpinSpeedTooltip"));
  p = obs_properties_add_float_slider(props, P_PART_CA_STRENGTH,
                                      obs_module_text("PrismIntensity"), 0.0,
                                      1.0, 0.01);
  obs_property_set_long_description(p,
                                    obs_module_text("PrismIntensityTooltip"));
}

// -------------------------------------------------------------------------------------------------
// Orton
// -------------------------------------------------------------------------------------------------

static void add_orton_properties(obs_properties_t *props) {
  obs_property_t *p;

  section_header(props, "Core");
  p = obs_properties_add_float_slider(
      props, P_ORTON_BLUR_RADIUS, obs_module_text("Spread"), 0.0, 50.0, 0.5);
  obs_property_set_long_description(p, obs_module_text("SpreadTooltip"));
  p = obs_properties_add_float_slider(
      props, P_ORTON_BLUR_STRENGTH, obs_module_text("Opacity"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("OpacityTooltip"));

  section_header(props, "Color Adjust");
  p = obs_properties_add_float_slider(props, P_ORTON_BRIGHTNESS,
                                      obs_module_text("Brightness"), -1.0, 1.0,
                                      0.01);
  obs_property_set_long_description(p, obs_module_text("BrightnessTooltip"));
  p = obs_properties_add_float_slider(
      props, P_ORTON_CONTRAST, obs_module_text("Contrast"), 0.1, 5.0, 0.05);
  obs_property_set_long_description(p, obs_module_text("ContrastTooltip"));
  p = obs_properties_add_float_slider(
      props, P_ORTON_SATURATION, obs_module_text("Saturation"), 0.0, 5.0, 0.05);
  obs_property_set_long_description(p, obs_module_text("SaturationTooltip"));
  p = obs_properties_add_float_slider(props, P_ORTON_HIGHLIGHT_BOOST,
                                      obs_module_text("HighlightBoost"), 0.0,
                                      5.0, 0.05);
  obs_property_set_long_description(p,
                                    obs_module_text("HighlightBoostTooltip"));

  section_header(props, "Blending");
  obs_property_t *orton_blend = obs_properties_add_list(
      props, P_ORTON_BLEND_MODE, obs_module_text("BlendMode"),
      OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
  obs_property_set_long_description(orton_blend,
                                    obs_module_text("BlendModeTooltip"));
  obs_property_list_add_int(orton_blend, obs_module_text("Standard"), 0);
  obs_property_list_add_int(orton_blend, obs_module_text("Vivid"), 1);
  obs_property_list_add_int(orton_blend, obs_module_text("LumaOnly"), 2);
}

// -------------------------------------------------------------------------------------------------
// Bloom
// -------------------------------------------------------------------------------------------------

static void add_bloom_properties(obs_properties_t *props) {
  obs_property_t *p;

  section_header(props, "Core");
  p = obs_properties_add_float_slider(
      props, P_BLOOM_THRESHOLD, obs_module_text("Threshold"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("ThresholdTooltip"));
  p = obs_properties_add_float_slider(
      props, P_BLOOM_INTENSITY, obs_module_text("Intensity"), 0.0, 10.0, 0.1);
  obs_property_set_long_description(p, obs_module_text("IntensityTooltip"));
  p = obs_properties_add_float_slider(
      props, P_BLOOM_RADIUS, obs_module_text("GlowRadius"), 0.0, 100.0, 1.0);
  obs_property_set_long_description(p, obs_module_text("GlowRadiusTooltip"));

  section_header(props, "Lens Dirt");
  p = obs_properties_add_float_slider(props, P_BLOOM_DUST_AMOUNT,
                                      obs_module_text("DustAmount"), 0.0, 2.0,
                                      0.01);
  obs_property_set_long_description(
      p, obs_module_text("BloomDustAmountTooltip"));
  p = obs_properties_add_float_slider(
      props, P_BLOOM_DUST_SCALE, obs_module_text("DustScale"), 1.0, 50.0, 0.5);
  obs_property_set_long_description(p, obs_module_text("DustScaleTooltip"));
  p = obs_properties_add_float_slider(props, P_BLOOM_HAZE_AMOUNT,
                                      obs_module_text("HazeAmount"), 0.0, 2.0,
                                      0.01);
  obs_property_set_long_description(p, obs_module_text("HazeAmountTooltip"));
  p = obs_properties_add_float_slider(props, P_BLOOM_DUST_CONTRAST,
                                      obs_module_text("DustContrast"), 0.0, 2.0,
                                      0.01);
  obs_property_set_long_description(p, obs_module_text("DustContrastTooltip"));
  p = obs_properties_add_float_slider(props, P_BLOOM_DUST_SPECULAR,
                                      obs_module_text("DustSpecular"), 0.0, 2.0,
                                      0.01);
  obs_property_set_long_description(p, obs_module_text("DustSpecularTooltip"));
  p = obs_properties_add_float_slider(props, P_BLOOM_DUST_DEPTH_LAYERS,
                                      obs_module_text("DustDepthLayers"), 1.0,
                                      5.0, 1.0);
  obs_property_set_long_description(p,
                                    obs_module_text("DustDepthLayersTooltip"));

  section_header(props, "Refraction");
  p = obs_properties_add_float_slider(props, P_BLOOM_REFRACT_AMOUNT,
                                      obs_module_text("RefractionAmount"), 0.0,
                                      1.0, 0.01);
  obs_property_set_long_description(p,
                                    obs_module_text("RefractionAmountTooltip"));
  p = obs_properties_add_float_slider(props, P_BLOOM_REFRACT_SCALE,
                                      obs_module_text("RefractionScale"), 0.1,
                                      10.0, 0.1);
  obs_property_set_long_description(p,
                                    obs_module_text("RefractionScaleTooltip"));

  section_header(props, "Ghost Reflections");
  p = obs_properties_add_float_slider(props, P_BLOOM_GHOST_AMOUNT,
                                      obs_module_text("GhostAmount"), 0.0, 2.0,
                                      0.01);
  obs_property_set_long_description(p, obs_module_text("GhostAmountTooltip"));
  p = obs_properties_add_float_slider(props, P_BLOOM_GHOST_SCALE,
                                      obs_module_text("GhostScale"), 0.1, 10.0,
                                      0.1);
  obs_property_set_long_description(p, obs_module_text("GhostScaleTooltip"));
  p = obs_properties_add_color_alpha(props, P_BLOOM_GHOST_COLOR,
                                     obs_module_text("GhostColor"));
  obs_property_set_long_description(p, obs_module_text("GhostColorTooltip"));

  section_header(props, "Advanced");
  p = obs_properties_add_float_slider(
      props, P_BLOOM_SOFTNESS, obs_module_text("Softness"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("SoftnessTooltip"));
  p = obs_properties_add_bool(props, P_BLOOM_HIGHLIGHTS_ONLY,
                              obs_module_text("HighlightsOnly"));
  obs_property_set_long_description(p,
                                    obs_module_text("HighlightsOnlyTooltip"));
  p = obs_properties_add_bool(props, P_BLOOM_ANAMORPHIC,
                              obs_module_text("AnamorphicBloom"));
  obs_property_set_long_description(p,
                                    obs_module_text("AnamorphicBloomTooltip"));

  section_header(props, "Color Tint");
  p = obs_properties_add_color_alpha(props, P_BLOOM_COLOR,
                                     obs_module_text("BloomColor"));
  obs_property_set_long_description(p, obs_module_text("BloomColorTooltip"));
}

// -------------------------------------------------------------------------------------------------
// Grit
// -------------------------------------------------------------------------------------------------

static void add_grit_properties(obs_properties_t *props) {
  obs_property_t *p;

  section_header(props, "Damage");
  p = obs_properties_add_float_slider(
      props, P_GRIT_DUST, obs_module_text("DustAmount"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("DustAmountTooltip"));
  p = obs_properties_add_float_slider(
      props, P_GRIT_DUST_SIZE, obs_module_text("DustSize"), 0.1, 5.0, 0.1);
  obs_property_set_long_description(p, obs_module_text("DustSizeTooltip"));
  p = obs_properties_add_float_slider(
      props, P_GRIT_SCRATCHES, obs_module_text("Scratches"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("ScratchesTooltip"));
  obs_property_t *grit_scratch_type = obs_properties_add_list(
      props, P_GRIT_SCRATCH_TYPE, obs_module_text("ScratchType"),
      OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
  obs_property_set_long_description(grit_scratch_type,
                                    obs_module_text("ScratchTypeTooltip"));
  obs_property_list_add_int(grit_scratch_type, obs_module_text("Horizontal"),
                            0);
  obs_property_list_add_int(grit_scratch_type, obs_module_text("Vertical"), 1);
  obs_property_list_add_int(grit_scratch_type, obs_module_text("Both"), 2);
  p = obs_properties_add_float_slider(
      props, P_GRIT_NOISE, obs_module_text("GrainIntensity"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p,
                                    obs_module_text("GrainIntensityTooltip"));
  p = obs_properties_add_float_slider(
      props, P_GRIT_FLARES, obs_module_text("FilmFlashes"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("FilmFlashesTooltip"));

  section_header(props, "Color & Adjust");
  p = obs_properties_add_color_alpha(props, P_GRIT_COLOR,
                                     obs_module_text("GrainColor"));
  obs_property_set_long_description(p, obs_module_text("GrainColorTooltip"));
  p = obs_properties_add_float_slider(
      props, P_GRIT_BRIGHTNESS, obs_module_text("Brightness"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("BrightnessTooltip"));
  p = obs_properties_add_float_slider(
      props, P_GRIT_CONTRAST, obs_module_text("Contrast"), 0.0, 5.0, 0.1);
  obs_property_set_long_description(p, obs_module_text("ContrastTooltip"));
  p = obs_properties_add_float_slider(
      props, P_GRIT_SATURATION, obs_module_text("Saturation"), 0.0, 5.0, 0.1);
  obs_property_set_long_description(p, obs_module_text("SaturationTooltip"));
  p = obs_properties_add_float_slider(
      props, P_GRIT_OPACITY, obs_module_text("OverallOpacity"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p,
                                    obs_module_text("OverallOpacityTooltip"));

  section_header(props, "Film Gate");
  p = obs_properties_add_float_slider(
      props, P_GRIT_INSTABILITY, obs_module_text("GateWeave"), 0.0, 2.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("GateWeaveTooltip"));

  section_header(props, "Blending");
  obs_property_t *grit_blend = obs_properties_add_list(
      props, P_GRIT_BLEND_MODE, obs_module_text("BlendMode"),
      OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
  obs_property_set_long_description(grit_blend,
                                    obs_module_text("BlendModeTooltip"));
  obs_property_list_add_int(grit_blend, obs_module_text("Multiply"), 0);
  obs_property_list_add_int(grit_blend, obs_module_text("Screen"), 1);
  obs_property_list_add_int(grit_blend, obs_module_text("Overlay"), 2);
  obs_property_list_add_int(grit_blend, obs_module_text("SoftLight"), 3);
  obs_property_list_add_int(grit_blend, obs_module_text("Lighten"), 4);
  obs_property_list_add_int(grit_blend, obs_module_text("HardLight"), 5);
}

// -------------------------------------------------------------------------------------------------
// Mirage
// -------------------------------------------------------------------------------------------------

static void add_mirage_properties(obs_properties_t *props) {
  obs_property_t *p;

  section_header(props, "Core");
  p = obs_properties_add_int_slider(props, P_MIR_COUNT,
                                    obs_module_text("SegmentCount"), 3, 12, 1);
  obs_property_set_long_description(p, obs_module_text("SegmentCountTooltip"));
  p = obs_properties_add_float_slider(
      props, P_MIR_RADIUS, obs_module_text("Radius"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("RadiusTooltip"));

  section_header(props, "Animation");
  p = obs_properties_add_float_slider(props, P_MIR_ROTATION,
                                      obs_module_text("RotationOffset"), 0.0,
                                      360.0, 1.0);
  obs_property_set_long_description(p,
                                    obs_module_text("RotationOffsetTooltip"));
  p = obs_properties_add_float_slider(props, P_MIR_ROT_SPEED,
                                      obs_module_text("RotationSpeed"), -10.0,
                                      10.0, 0.1);
  obs_property_set_long_description(p, obs_module_text("RotationSpeedTooltip"));

  section_header(props, "Advanced");
  p = obs_properties_add_float_slider(
      props, P_MIR_CENTER_MASK, obs_module_text("CenterMask"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("CenterMaskTooltip"));
  p = obs_properties_add_bool(props, P_MIR_PARALLEL,
                              obs_module_text("ParallelMode"));
  obs_property_set_long_description(p, obs_module_text("ParallelModeTooltip"));
}

// -------------------------------------------------------------------------------------------------
// Spot
// -------------------------------------------------------------------------------------------------

static void add_spot_properties(obs_properties_t *props) {
  obs_property_t *p;

  section_header(props, "Core");
  p = obs_properties_add_float_slider(
      props, P_SPOT_RADIUS, obs_module_text("ClearRadius"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("ClearRadiusTooltip"));
  p = obs_properties_add_float_slider(
      props, P_SPOT_SOFTNESS, obs_module_text("EdgeSoftness"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("EdgeSoftnessTooltip"));

  section_header(props, "Edge Effects");
  p = obs_properties_add_float_slider(
      props, P_SPOT_ZOOM_STRENGTH, obs_module_text("EdgeZoom"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("EdgeZoomTooltip"));
  p = obs_properties_add_float_slider(props, P_SPOT_BLUR_AMOUNT,
                                      obs_module_text("EdgeBlurAmount"), 0.0,
                                      1.0, 0.01);
  obs_property_set_long_description(p,
                                    obs_module_text("EdgeBlurAmountTooltip"));
  p = obs_properties_add_float_slider(props, P_SPOT_BLUR_RADIUS,
                                      obs_module_text("EdgeBlurRadius"), 0.0,
                                      20.0, 0.1);
  obs_property_set_long_description(p,
                                    obs_module_text("EdgeBlurRadiusTooltip"));

  section_header(props, "Optics");
  obs_property_t *spot_blend = obs_properties_add_list(
      props, P_SPOT_BLEND_MODE, obs_module_text("SpotOpticType"),
      OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
  obs_property_set_long_description(spot_blend,
                                    obs_module_text("SpotOpticTypeTooltip"));
  obs_property_list_add_int(spot_blend, obs_module_text("SpotSoftDiffusion"), 0);
  obs_property_list_add_int(spot_blend, obs_module_text("SpotDiopter"), 1);
  obs_property_list_add_int(spot_blend, obs_module_text("SpotRadialZoom"), 2);
}

// -------------------------------------------------------------------------------------------------
// Pinhole
// -------------------------------------------------------------------------------------------------

static void add_pinhole_properties(obs_properties_t *props) {
  obs_property_t *p;

  section_header(props, "Core");
  p = obs_properties_add_float_slider(
      props, P_PINHOLE_SOFTNESS, obs_module_text("Softness"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p,
                                    obs_module_text("PinholeSoftnessTooltip"));
  section_header(props, "Color & Vignette");
  p = obs_properties_add_float_slider(
      props, P_PINHOLE_VIGNETTE, obs_module_text("Vignette"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p,
                                    obs_module_text("PinholeVignetteTooltip"));
  p = obs_properties_add_float_slider(
      props, P_PINHOLE_SEPIA, obs_module_text("SepiaTint"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("PinholeSepiaTooltip"));

  section_header(props, "Surface Texture");
  p = obs_properties_add_float_slider(props, P_PINHOLE_GRAIN,
                                      obs_module_text("GrainIntensity"), 0.0,
                                      1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("PinholeGrainTooltip"));
}

// -------------------------------------------------------------------------------------------------
// Polaroid
// -------------------------------------------------------------------------------------------------

static void add_polaroid_properties(obs_properties_t *props) {
  obs_property_t *p;

  section_header(props, "Core");
  p = obs_properties_add_float_slider(
      props, P_POLAROID_SOFTNESS, obs_module_text("Softness"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p,
                                    obs_module_text("PolaroidSoftnessTooltip"));
  p = obs_properties_add_float_slider(
      props, P_POLAROID_VIGNETTE, obs_module_text("Vignette"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p,
                                    obs_module_text("PolaroidVignetteTooltip"));

  section_header(props, "Color & Contrast");
  p = obs_properties_add_float_slider(props, P_POLAROID_COLOR_SHIFT,
                                      obs_module_text("ColorShift"), 0.0, 1.0,
                                      0.01);
  obs_property_set_long_description(
      p, obs_module_text("PolaroidColorShiftTooltip"));
  p = obs_properties_add_float_slider(
      props, P_POLAROID_CONTRAST, obs_module_text("Contrast"), 0.0, 2.0, 0.01);
  obs_property_set_long_description(p,
                                    obs_module_text("PolaroidContrastTooltip"));

  section_header(props, "Development");
  p = obs_properties_add_float_slider(props, P_POLAROID_DEVELOP,
                                      obs_module_text("DevelopAmount"), 0.0,
                                      1.0, 0.01);
  obs_property_set_long_description(p,
                                    obs_module_text("PolaroidDevelopTooltip"));
  p = obs_properties_add_float_slider(props, P_POLAROID_DEVELOP_CENTER,
                                      obs_module_text("DevelopCenter"), 0.0,
                                      1.0, 0.01);
  obs_property_set_long_description(
      p, obs_module_text("PolaroidDevelopCenterTooltip"));

  section_header(props, "Film Characteristics");
  p = obs_properties_add_color_alpha(props, P_POLAROID_BASE_COLOR,
                                     obs_module_text("BaseColor"));
  obs_property_set_long_description(
      p, obs_module_text("PolaroidBaseColorTooltip"));
  p = obs_properties_add_float_slider(props, P_POLAROID_BLEED,
                                      obs_module_text("ChemicalBleed"), 0.0,
                                      1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("PolaroidBleedTooltip"));
  p = obs_properties_add_float_slider(props, P_POLAROID_GRAIN,
                                      obs_module_text("GrainIntensity"), 0.0,
                                      1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("PolaroidGrainTooltip"));
}

// -------------------------------------------------------------------------------------------------
// Daguerreotype
// -------------------------------------------------------------------------------------------------

static void add_daguerreotype_properties(obs_properties_t *props) {
  obs_property_t *p;

  section_header(props, "Core");
  p = obs_properties_add_float_slider(
      props, P_DAG_CONTRAST, obs_module_text("Contrast"), 0.0, 2.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("DagContrastTooltip"));
  p = obs_properties_add_float_slider(
      props, P_DAG_TINT, obs_module_text("SilverTint"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("DagTintTooltip"));

  section_header(props, "Surface");
  p = obs_properties_add_float_slider(
      props, P_DAG_GRAIN, obs_module_text("GrainIntensity"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("DagGrainTooltip"));
  p = obs_properties_add_float_slider(
      props, P_DAG_VIGNETTE, obs_module_text("Vignette"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("DagVignetteTooltip"));
  p = obs_properties_add_float_slider(
      props, P_DAG_EDGE_BLUR, obs_module_text("EdgeBlur"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("DagEdgeBlurTooltip"));
}

// -------------------------------------------------------------------------------------------------
// Technicolor
// -------------------------------------------------------------------------------------------------

static void add_technicolor_properties(obs_properties_t *props) {
  obs_property_t *p;

  section_header(props, "Core");
  p = obs_properties_add_float_slider(props, P_TECH_STRENGTH,
                                      obs_module_text("ProcessStrength"), 0.0,
                                      1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("TechStrengthTooltip"));
  p = obs_properties_add_float_slider(
      props, P_TECH_SATURATION, obs_module_text("Saturation"), 0.0, 3.0, 0.01);
  obs_property_set_long_description(p,
                                    obs_module_text("TechSaturationTooltip"));

  section_header(props, "Color Split");
  p = obs_properties_add_float_slider(props, P_TECH_OFFSET,
                                      obs_module_text("ColorSplitOffset"), 0.0,
                                      1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("TechOffsetTooltip"));
  p = obs_properties_add_float_slider(
      props, P_TECH_GLOW, obs_module_text("HighlightGlow"), 0.0, 1.0, 0.01);
  obs_property_set_long_description(p, obs_module_text("TechGlowTooltip"));
}

// -------------------------------------------------------------------------------------------------
// Wet Lens: physical-looking refraction on stationary glass
// -------------------------------------------------------------------------------------------------

static void add_wet_lens_properties(obs_properties_t *props) {
  obs_property_t *p;
  section_header(props, "Water on Glass");
  p = obs_properties_add_float_slider(props, P_WET_DENSITY,
      "Droplet Coverage", 0.0, 1.0, 0.01);
  obs_property_set_long_description(p,
      "Fraction of possible droplet sites. Zero leaves the source unchanged.");
  p = obs_properties_add_float_slider(props, P_WET_SIZE,
      "Droplet Size", 0.25, 1.25, 0.01);
  obs_property_set_long_description(p,
      "Relative bead radius on the lens plane, independent of the picture behind it.");
  p = obs_properties_add_float_slider(props, P_WET_STRETCH,
      "Gravity Stretch", 0.0, 0.8, 0.01);
  obs_property_set_long_description(p,
      "Makes beads slightly longer vertically; this does not simulate flowing water.");
  section_header(props, "Optics");
  p = obs_properties_add_float_slider(props, P_WET_REFRACTION,
      "Lens Refraction", 0.0, 1.0, 0.01);
  obs_property_set_long_description(p,
      "Bends the actual source behind each curved droplet.");
  p = obs_properties_add_float_slider(props, P_WET_GLINT,
      "Captured Light Response", 0.0, 2.0, 0.01);
  obs_property_set_long_description(p,
      "Rim glints respond to light in the camera image, not a painted-on glow.");
  p = obs_properties_add_float_slider(props, P_WET_CHROMATIC,
      "Edge Chromatic Separation", 0.0, 1.0, 0.01);
  obs_property_set_long_description(p,
      "Small spectral split at refracting droplet edges.");
  section_header(props, "Distribution");
  p = obs_properties_add_int_slider(props, P_WET_SEED,
      "Droplet Pattern", 0, 100, 1);
  obs_property_set_long_description(p,
      "Changes the deterministic placement without making droplets drift every frame.");
}

// -------------------------------------------------------------------------------------------------
// Phone Wet Lens — broad out-of-focus water, highlight ghosts and streaks
// -------------------------------------------------------------------------------------------------
static void add_phone_wet_properties(obs_properties_t *props) {
  obs_property_t *p;
  section_header(props, "Moisture and Shape");
  p = obs_properties_add_float_slider(props, P_PHONE_COVERAGE,
      "Wet Surface Coverage", 0.0, 1.0, 0.01);
  obs_property_set_long_description(p,
      "Scattered wet patches and their optical effects. At zero the original source is preserved.");
  p = obs_properties_add_float_slider(props, P_PHONE_SCALE,
      "Water Patch Scale", 0.5, 1.8, 0.01);
  obs_property_set_long_description(p,
      "Size of out-of-focus water distortions; these are not sharply outlined beads.");
  p = obs_properties_add_float_slider(props, P_PHONE_ARCS,
      "Wet Contour Strength", 0.0, 1.5, 0.01);
  obs_property_set_long_description(p,
      "Broad refractive meniscus contours that react to incident light.");
  section_header(props, "Light Through Water");
  p = obs_properties_add_float_slider(props, P_PHONE_REFRACTION,
      "Broad Refraction", 0.0, 1.0, 0.01);
  obs_property_set_long_description(p,
      "Bends the actual captured image along the changing water thickness.");
  p = obs_properties_add_float_slider(props, P_PHONE_RIBBONS,
      "Bright-Light Ribbons", 0.0, 2.0, 0.01);
  obs_property_set_long_description(p,
      "Spreads light from real lamps and windows into irregular, water-shaped streaks.");
  p = obs_properties_add_float_slider(props, P_PHONE_REACH,
      "Ribbon Reach", 0.0, 1.0, 0.01);
  obs_property_set_long_description(p,
      "Horizontal reach of captured light; zero removes extended streak length.");
  p = obs_properties_add_float_slider(props, P_PHONE_GLOW,
      "Veiling Glow", 0.0, 1.0, 0.01);
  obs_property_set_long_description(p,
      "Localized out-of-focus scatter revealed by bright objects, not uniform fog.");
  section_header(props, "Orientation and Variation");
  p = obs_properties_add_float_slider(props, P_PHONE_ANGLE,
      "Light Streak Angle", -80.0, 80.0, 1.0);
  obs_property_set_long_description(p,
      "Rotate the directional optical streaks without turning the whole picture.");
  p = obs_properties_add_int_slider(props, P_PHONE_SEED,
      "Wet Surface Pattern", 0, 100, 1);
  obs_property_set_long_description(p,
      "Stable distribution. Changing it regenerates the random shape arrangement.");
}

// -------------------------------------------------------------------------------------------------
// Presets
// -------------------------------------------------------------------------------------------------

static void add_preset_properties(obs_properties_t *props) {
  obs_property_t *presets =
      obs_properties_add_list(props, UI_PRESET_LIST, obs_module_text("Preset"),
                              OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
  obs_property_set_long_description(presets, obs_module_text("PresetTooltip"));
  obs_property_set_modified_callback(presets, load_preset_callback);
}

static void populate_preset_entries(obs_property_t *presets, int mode_index) {
  obs_property_list_add_string(presets, obs_module_text("SelectPreset"), "");

  // Both call sites pass a value already clamped to [0, NUM_MODES) by
  // mode_from_settings(). No fallback needed; the prior else branch
  // iterating all modes was dead code.
  if (strategies[mode_index].populate_presets)
    strategies[mode_index].populate_presets(presets);
}
