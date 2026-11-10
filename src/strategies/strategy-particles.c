#include "../obs-utils.h"

void particles_cache(struct lens_data *filter, gs_effect_t *effect) {
  GET_PARAM_SAFE(filter->mode_params.particles.density, effect, P_PART_DENSITY);
  GET_PARAM_SAFE(filter->mode_params.particles.size, effect, P_PART_SIZE);
  GET_PARAM_SAFE(filter->mode_params.particles.size_var, effect, P_PART_SIZE_VAR);
  GET_PARAM_SAFE(filter->mode_params.particles.speed, effect, P_PART_SPEED);
  GET_PARAM_SAFE(filter->mode_params.particles.wind_x, effect, P_PART_WIND_X);
  GET_PARAM_SAFE(filter->mode_params.particles.wind_y, effect, P_PART_WIND_Y);
  GET_PARAM_SAFE(filter->mode_params.particles.wobble, effect, P_PART_WOBBLE);
  GET_PARAM_SAFE(filter->mode_params.particles.color_start, effect,
                 P_PART_COLOR_START);
  GET_PARAM_SAFE(filter->mode_params.particles.color_end, effect,
                 P_PART_COLOR_END);
  GET_PARAM_SAFE(filter->mode_params.particles.source_color, effect,
                 P_PART_SOURCE_COLOR);
  GET_PARAM_SAFE(filter->mode_params.particles.source_influence, effect,
                 P_PART_SOURCE_INFLUENCE);
  GET_PARAM_SAFE(filter->mode_params.particles.source_brightness, effect,
                 P_PART_SOURCE_BRIGHTNESS);
  GET_PARAM_SAFE(filter->mode_params.particles.source_threshold, effect,
                 P_PART_SOURCE_THRESHOLD);
  GET_PARAM_SAFE(filter->mode_params.particles.source_strength, effect,
                 P_PART_SOURCE_STRENGTH);
  GET_PARAM_SAFE(filter->mode_params.particles.focus_x, effect, P_PART_FOCUS_X);
  GET_PARAM_SAFE(filter->mode_params.particles.focus_y, effect, P_PART_FOCUS_Y);
  GET_PARAM_SAFE(filter->mode_params.particles.focus_strength, effect,
                 P_PART_FOCUS_STRENGTH);
  GET_PARAM_SAFE(filter->mode_params.particles.blur_edge, effect,
                 P_PART_BLUR_EDGE);
  GET_PARAM_SAFE(filter->mode_params.particles.use_poly, effect,
                 P_PART_USE_POLY);
  GET_PARAM_SAFE(filter->mode_params.particles.poly_sides, effect,
                 P_PART_POLY_SIDES);
  GET_PARAM_SAFE(filter->mode_params.particles.poly_rot, effect,
                 P_PART_POLY_ROT);
  GET_PARAM_SAFE(filter->mode_params.particles.poly_rot_speed, effect,
                 P_PART_POLY_ROT_SPEED);
  GET_PARAM_SAFE(filter->mode_params.particles.part_ca_strength, effect,
                 P_PART_CA_STRENGTH);
}

void particles_update(struct lens_data *filter, obs_data_t *settings) {
  SET_PARAM_FLOAT(filter->mode_params.particles.density, clamp_float(
      (float)obs_data_get_double(settings, P_PART_DENSITY), 1.0f, 40.0f));
  SET_PARAM_FLOAT(filter->mode_params.particles.size, clamp_float(
      (float)obs_data_get_double(settings, P_PART_SIZE), 0.001f, 0.1f));
  SET_PARAM_FLOAT(filter->mode_params.particles.size_var, clamp_float(
      (float)obs_data_get_double(settings, P_PART_SIZE_VAR), 0.0f, 1.0f));
  SET_PARAM_FLOAT(filter->mode_params.particles.speed, clamp_float(
      (float)obs_data_get_double(settings, P_PART_SPEED), 0.0f, 5.0f));
  SET_PARAM_FLOAT(filter->mode_params.particles.wind_x, clamp_float(
      (float)obs_data_get_double(settings, P_PART_WIND_X), -1.0f, 1.0f));
  SET_PARAM_FLOAT(filter->mode_params.particles.wind_y, clamp_float(
      (float)obs_data_get_double(settings, P_PART_WIND_Y), -1.0f, 1.0f));
  SET_PARAM_FLOAT(filter->mode_params.particles.wobble, clamp_float(
      (float)obs_data_get_double(settings, P_PART_WOBBLE), 0.0f, 1.0f));

  uint32_t c_val = (uint32_t)obs_data_get_int(settings, P_PART_COLOR_START);
  struct vec4 _c_vec_color_start;
  emulens_color_to_vec4(&_c_vec_color_start, c_val);
  SET_PARAM_VEC4(filter->mode_params.particles.color_start, &_c_vec_color_start);
  c_val = (uint32_t)obs_data_get_int(settings, P_PART_COLOR_END);
  struct vec4 _c_vec_color_end;
  emulens_color_to_vec4(&_c_vec_color_end, c_val);
  SET_PARAM_VEC4(filter->mode_params.particles.color_end, &_c_vec_color_end);

  SET_PARAM_FLOAT(filter->mode_params.particles.source_color, (float)obs_data_get_bool(settings, P_PART_SOURCE_COLOR));
  SET_PARAM_FLOAT(filter->mode_params.particles.source_influence, clamp_float((float)obs_data_get_double(settings, P_PART_SOURCE_INFLUENCE),
                   0.0f, 1.0f));
  SET_PARAM_FLOAT(filter->mode_params.particles.source_brightness, (float)obs_data_get_bool(settings, P_PART_SOURCE_BRIGHTNESS));
  SET_PARAM_FLOAT(filter->mode_params.particles.source_threshold, clamp_float((float)obs_data_get_double(settings, P_PART_SOURCE_THRESHOLD),
                   0.0f, 1.0f));
  SET_PARAM_FLOAT(filter->mode_params.particles.source_strength, clamp_float((float)obs_data_get_double(settings, P_PART_SOURCE_STRENGTH),
                   0.0f, 5.0f));

  SET_PARAM_FLOAT(filter->mode_params.particles.focus_x, clamp_float(
      (float)obs_data_get_double(settings, P_PART_FOCUS_X), 0.0f, 1.0f));
  SET_PARAM_FLOAT(filter->mode_params.particles.focus_y, clamp_float(
      (float)obs_data_get_double(settings, P_PART_FOCUS_Y), 0.0f, 1.0f));
  SET_PARAM_FLOAT(filter->mode_params.particles.focus_strength, clamp_float(
      (float)obs_data_get_double(settings, P_PART_FOCUS_STRENGTH), 0.0f, 1.0f));
  SET_PARAM_FLOAT(filter->mode_params.particles.blur_edge, clamp_float(
      (float)obs_data_get_double(settings, P_PART_BLUR_EDGE), 0.0f, 1.0f));

  SET_PARAM_FLOAT(filter->mode_params.particles.use_poly, (float)obs_data_get_bool(settings, P_PART_USE_POLY));
  SET_PARAM_FLOAT(filter->mode_params.particles.poly_sides, clamp_float((float)obs_data_get_double(settings, P_PART_POLY_SIDES), 3.0f, 10.0f));
  SET_PARAM_FLOAT(filter->mode_params.particles.poly_rot, clamp_float(
      (float)obs_data_get_double(settings, P_PART_POLY_ROT), 0.0f, 360.0f));
  SET_PARAM_FLOAT(filter->mode_params.particles.poly_rot_speed, clamp_float((float)obs_data_get_double(settings, P_PART_POLY_ROT_SPEED),
                  -10.0f, 10.0f));
  SET_PARAM_FLOAT(filter->mode_params.particles.part_ca_strength, clamp_float(
      (float)obs_data_get_double(settings, P_PART_CA_STRENGTH), 0.0f, 1.0f));
}

gs_texture_t *particles_render(struct lens_data *filter, gs_effect_t *effect,
                               gs_texture_t *source_tex, uint32_t width,
                               uint32_t height) {
  UNUSED_PARAMETER(effect);
  UNUSED_PARAMETER(source_tex);
  return render_passthrough_to_texrender(
      filter->texrenders[emulens_atomic_load(&filter->cur_write)], filter->tech.particles, filter,
      source_tex, width, height);
}

void particles_destroy_cpu(struct lens_data *filter) { (void)filter; }
void particles_destroy(struct lens_data *filter) { (void)filter; }

void particles_get_defaults(obs_data_t *settings) {
  obs_data_set_default_double(settings, P_PART_DENSITY, 18.0);
  obs_data_set_default_double(settings, P_PART_SIZE, 0.004);
  obs_data_set_default_double(settings, P_PART_SIZE_VAR, 0.55);
  obs_data_set_default_double(settings, P_PART_SPEED, 0.25);
  obs_data_set_default_double(settings, P_PART_WIND_X, 0.02);
  obs_data_set_default_double(settings, P_PART_WIND_Y, -0.01);
  obs_data_set_default_double(settings, P_PART_WOBBLE, 0.16);
  obs_data_set_default_int(settings, P_PART_COLOR_START, 0x66FFFFFF);
  obs_data_set_default_int(settings, P_PART_COLOR_END, 0x18FFFFFF);
  obs_data_set_default_bool(settings, P_PART_SOURCE_COLOR, false);
  obs_data_set_default_double(settings, P_PART_SOURCE_INFLUENCE, 0.0);
  obs_data_set_default_bool(settings, P_PART_SOURCE_BRIGHTNESS, false);
  obs_data_set_default_double(settings, P_PART_SOURCE_THRESHOLD, 0.70);
  obs_data_set_default_double(settings, P_PART_SOURCE_STRENGTH, 1.0);
  obs_data_set_default_double(settings, P_PART_FOCUS_X, 0.5);
  obs_data_set_default_double(settings, P_PART_FOCUS_Y, 0.5);
  obs_data_set_default_double(settings, P_PART_FOCUS_STRENGTH, 0.0);
  obs_data_set_default_double(settings, P_PART_BLUR_EDGE, 0.65);
  obs_data_set_default_bool(settings, P_PART_USE_POLY, false);
  obs_data_set_default_int(settings, P_PART_POLY_SIDES, 6);
  obs_data_set_default_double(settings, P_PART_POLY_ROT, 0.0);
  obs_data_set_default_double(settings, P_PART_POLY_ROT_SPEED, 0.0);
  obs_data_set_default_double(settings, P_PART_CA_STRENGTH, 0.0);
}

void particles_populate_presets(obs_property_t *list) {
  ADD_PRESET_ENTRY(list, "PartDefault", "default_particles",
                    "PartDefaultDesc");
  ADD_PRESET_ENTRY(list, "PartDust", "part_dust",
                    "PartDustDesc");
  ADD_PRESET_ENTRY(list, "PartMagic", "part_magic",
                    "PartMagicDesc");
  ADD_PRESET_ENTRY(list, "PartSnow", "part_snow",
                    "PartSnowDesc");
}
