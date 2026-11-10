#!/usr/bin/env python3
"""
Render pipeline invariant tests for emuLensFX.

Verifies lens_render() in lens-core.c contains required safety guards,
fallback paths, and dispatch patterns.
"""

import os
import re
import sys

REPO = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
CORE_PATH = os.path.join(REPO, "src", "lens-core.c")
DATA_PATH = os.path.join(REPO, "src", "lens-data.h")
INLINE_PATH = os.path.join(REPO, "src", "lens-inline.h")
UTILS_PATH = os.path.join(REPO, "src", "obs-utils.c")

errors = []


def read_file(path):
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def check():
    text = read_file(CORE_PATH) + "\n" + read_file(DATA_PATH) + "\n"
    text += read_file(INLINE_PATH) + "\n"
    try:
        text += read_file(UTILS_PATH)
    except FileNotFoundError:
        pass

    # 1. Must have null-effect guard
    if re.search(r'if\s*\(!effect\s*\)', text):
        print("  PASS: Null-effect guard present")
    elif re.search(r'if\s*\(!filter->effect\s*\)', text):
        print("  PASS: filter->effect null guard present")
    else:
        errors.append("No null-effect guard in lens_render")

    # 2. Must skip when base dimensions are zero
    if re.search(r'base_w\s*==\s*0\s*\|\|\s*base_h\s*==\s*0', text):
        print("  PASS: Zero base dimension guard present")
    else:
        errors.append("No zero-base-dimension guard in lens_render")

    # 3. Must call recreate_texrenders_if_needed on dimension change
    if re.search(r'recreate_texrenders_if_needed', text):
        print("  PASS: recreate_texrenders_if_needed called")
    else:
        errors.append("lens_render does not call recreate_texrenders_if_needed")

    # 4. Must handle resize failure after begin()
    if re.search(r'res\s*==\s*TEXRENDER_FAILED', text):
        print("  PASS: Resize failure guard present")
    else:
        errors.append("No TEXRENDER_FAILED check after resize")

    # 5. Must call get_input_source (formerly capture_input_texture, now
    # extracted to src/obs-utils.c).
    if re.search(r'get_input_source\s*\(', text):
        print("  PASS: get_input_source called")
    else:
        errors.append("lens_render does not call get_input_source")

    # 6. Must not fabricate a blank input when the filter target is
    # unavailable. The guard now lives in obs-utils.c's get_input_source.
    # Accept both brace and brace-less if forms.
    if re.search(
        r'obs_filter_get_target\([^)]*\).*?if\s*\(!target(?:\s*\|\|\s*target\s*==\s*filter->context)?\)(\s+|\s*\{\s*)return\s+(NULL|false);?',
        text, re.S,
    ):
        print("  PASS: Null target capture guard present")
    else:
        errors.append("get_input_source does not bail when target is missing")

    # 7. Must skip frame when input capture fails
    if re.search(r'!source_tex\s*\)', text):
        print("  PASS: Null source texture guard present")
    else:
        errors.append("No null-source-texture guard in lens_render")

    # 8. Offscreen passes may establish a local projection, but the final
    # OBS-facing composite must preserve the scene-item transform and bind only
    # uniforms/textures.
    if re.search(r'bind_common_parameters', text):
        print("  PASS: offscreen bind_common_parameters helper is present")
    else:
        errors.append("offscreen render path does not expose bind_common_parameters")
    if re.search(r'bind_common_uniforms', text):
        print("  PASS: transform-preserving uniform binder is present")
    else:
        errors.append("final composite has no transform-preserving uniform binder")

    # 9. Must have strategy dispatch
    if re.search(r'strat->render', text):
        print("  PASS: Strategy render dispatch present")
    else:
        errors.append("No strategy render dispatch in lens_render")

    # 10. Must have Draw technique fallback
    if re.search(r'filter->tech\.draw', text):
        print("  PASS: Draw technique fallback path present")
    else:
        errors.append("No Draw technique fallback in lens_render")

    # 11. Must guard failed technique pass begin calls
    if re.search(r'if\s*\(!gs_technique_begin_pass\(tech,\s*i\)\).*?(continue|break);', text, re.S):
        print("  PASS: Technique pass begin guard present")
    else:
        errors.append("lens_run_technique_quad does not guard gs_technique_begin_pass failure")

    # 12. Must have at least one skip_video_filter guard for pre-begin failure
    count = len(re.findall(r'obs_source_skip_video_filter', text))
    if count >= 1:
        print(f"  PASS: {count} skip_video_filter guard(s) present")
    else:
        errors.append(f"Too few skip_video_filter guards ({count})")

    # 13. Dimension-preserving filters must let libobs inherit dimensions from
    # the filter target.  Registering custom get_width/get_height callbacks and
    # calling obs_source_get_width(parent/target) from them can recurse when
    # multiple filters are stacked.
    info = re.search(
        r"struct obs_source_info emulens_filter_info\s*=\s*\{(?P<body>.*?)\n\};",
        text,
        re.S,
    )
    info_text = info.group("body") if info else ""
    if ".get_width" in info_text or ".get_height" in info_text:
        errors.append("Dimension-preserving filter registers recursive width/height callbacks")
    else:
        print("  PASS: Filter inherits width/height from libobs target chain")

    if re.search(r"static\s+uint32_t\s+lens_get_(width|height)\s*\(", text):
        errors.append("Obsolete lens_get_width/lens_get_height callback remains")
    elif "lens_get_render_size(filter, &base_w, &base_h)" not in text:
        errors.append("Render path does not resolve target dimensions in video_render")
    else:
        print("  PASS: Dimensions are resolved only from the render-scoped target")

    # 14. Final composite must present the completed strategy texture through
    # OBS's own base Draw effect. Re-running the finished texture through the
    # EmuLens effect can accidentally apply stale uniforms or custom state.
    base_composite = re.search(
        r'composite_tex\s*=\s*final_tex\s*\?\s*final_tex\s*:\s*source_tex;'
        r'.*?obs_get_base_effect\(OBS_EFFECT_DEFAULT\)'
        r'.*?gs_effect_get_param_by_name\(copy_effect,\s*"image"\)'
        r'.*?gs_effect_set_texture(?:_srgb)?\(image_param,\s*composite_tex\)'
        r'.*?gs_draw_sprite\(composite_tex,\s*0,\s*target_w,\s*target_h\)',
        text,
        re.S,
    )
    if base_composite:
        print("  PASS: Final composite uses OBS base Draw with the strategy texture")
    else:
        errors.append("Final composite does not use OBS base Draw correctly")

    # 15. Shader params must be rebound every frame. OBS/OpenGL can lose effect
    # param bindings between passes/frames.
    render_body = re.search(r'static void lens_render\(.*?\n\}', text, re.S)
    render_text = render_body.group(0) if render_body else ""
    rebinds = re.search(
        r'strat->update_settings\(filter,\s*settings\);',
        render_text,
    )
    if rebinds:
        print("  PASS: lens_render rebinds strategy settings every frame")
    else:
        errors.append("lens_render does not clearly rebind strategy settings every frame")

    # 16. Custom-draw render callbacks can be probed outside an active graphics
    # context by OBS preview/property paths. lens_render must fail closed before
    # any gs_* calls in that case; entering graphics inside video_render is not
    # allowed because OBS owns the render context lifecycle.
    context_guard = re.search(
        r'if\s*\(!gs_get_context\(\)\)\s*\{\s*obs_source_skip_video_filter\(filter->context\);\s*return;\s*\}',
        render_text,
        re.S,
    )
    forbidden_enter = re.search(r'static void lens_render\(.*?obs_enter_graphics\(', text, re.S)
    first_graphics_call = re.search(r'\b(?:gs_|detect_gpu_capabilities|get_input_source|recreate_texrenders_if_needed)\w*\s*\(', render_text)
    if not context_guard:
        errors.append("lens_render does not guard inactive graphics context before gs_* calls")
    elif forbidden_enter:
        errors.append("lens_render enters graphics context instead of failing closed")
    elif first_graphics_call and context_guard.start() > first_graphics_call.start():
        errors.append("lens_render graphics-context guard appears after graphics calls")
    else:
        print("  PASS: lens_render skips when OBS has no active graphics context")


def main():
    print("=" * 60)
    print("emuLensFX Render Pipeline Invariant Test")
    print("=" * 60)
    check()
    print("-" * 60)
    if errors:
        for e in errors:
            print(f"  FAIL: {e}")
        sys.exit(1)
    print("ALL CHECKS PASSED")


if __name__ == "__main__":
    main()
