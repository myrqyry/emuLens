# EmuLensFX Changelog

## [Unreleased]

### Rendering and optical behavior

- Added **Phone Wet Lens** as a separate seventeenth effect mode: broad
  near-lens water contours, live-image refraction, highlight-driven
  directional ribbons and localized veiling glare. Existing Wet Lens
  and Dusty Lens effects are preserved. Three presets support restrained,
  streetlight and heavy-water looks. A real OBS/OpenGL test verifies
  black-scene darkness and independent control responsiveness. Performance
  and photographic accuracy remain unverified.

- Added a separate **Wet Lens** optical mode: stationary curved water
  beads that refract the actual frame, gather captured-light rim glints,
  and allow restrained chromatic separation. Two presets: Clean Raindrops
  and Night Window. The earlier Dusty Lens Bloom implementation is unchanged.
  OBS/OpenGL pixel tests verify responsiveness but do not yet establish
  live-camera or real-time performance.

- Vidicon uses temporal trail-energy feedback with per-channel lag and a
  perceptual, frame-rate-independent Trail Decay control; `1.00` is the
  explicit indefinite-hold endpoint.
- Starburst ray reach follows highlight energy, so stronger point highlights
  generate longer and brighter diffraction rays.
- Center Spot keeps a clear center and exposes Soft Diffusion, Diopter, and
  Radial Zoom outer-optic constructions.
- Mirage/Kaleido mirrors out-of-bounds sampling at large radii rather than
  dropping invalid samples to black.
- Particles compensate energy as size increases and use bounded compositing to
  preserve separation under dense overlap.
- Bloom uses a multilevel bright-pass pyramid with fixed lens-space
  contamination, light-gated scatter/veiling glare/glints, gradient-driven
  refraction, ghost reflections, and optional anamorphic response.
- Bloom multipass rendering rebinds mode-specific settings after common
  parameter binding so live controls remain effective across extraction,
  pyramid/blur, and combine passes.

### Presets and controls

- 61 preset files cover 17 effect modes (including Wet Lens and Phone Wet Lens), with an explicit default preset for every mode.
- `Bloom: Dusty Lens` provides an optical-contamination starting point where
  grime remains dormant until bright light reveals it.
- Vidicon defaults target natural tube persistence while `vidicon_soft` and
  `vidicon_dream` occupy shorter and longer regions of the perceptual decay
  scale.
- Bloom terminology distinguishes fixed lens contamination from film-stock
  dirt; Veiling Glare, Dust Specular Glint, Lens Surface Layers, and
  contamination-driven refraction describe the optical response directly.

### Runtime and validation

- The OBS runtime harness validates rendered pixels for recovered effect
  behavior, including temporal Vidicon trails, large-radius Mirage, optical
  Center Spot modes, size-aware particles, Starburst highlight response, and
  Dusty Lens light interaction.
- Expected per-frame rendering does not emit Bloom entry/exit log spam;
  render-path logging is reserved for actionable failures.
- The standalone suite contains 49 CTest entries; the optional
  `PythonIntegrationTest` skips when `obspython` is unavailable.
- Release-hardening work adds source archive privacy verification, installer rollback regression tests, a real version banner and separate platform build/artifact checks. This does not constitute Windows/macOS runtime validation.

## [0.5.0] - 2026-05-12

### 🏛️ The "Museum of Historical Quirks" Update

#### **New Effects**
- **Pinhole Camera**: Authentic simulation of lensless photography.
- **Polaroid Film**: Chemical emulsion simulation with iconic color shifts.
- **Daguerreotype**: Metallic high-contrast silver-plate photography.
  - Extreme metallic contrast and silver halide grain.
  - Deep vignetting and intense corner focus falloff.
  - Realistic silver-mirroring edge artifacts.
- **Technicolor 2-Strip**: Early cinema dye-transfer recreation.
  - Authentic Red-Cyan restricted color palette.
  - Vivid saturation and highlight "dye spread" glow.
  - Simulated physical strip misalignment (Red/Green color splitting).

#### **Perfected Vidicon Simulation**
- **Red Phosphor Lag**: Channel-specific decay for authentic ghosting.
- **Comet-Tailing**: Horizontal scanning smear for moving highlights.
- **Tube Noise**: Electronic vacuum-tube gain snow in trails.

#### **Visual & UI Enhancements**
- **Iconic Interface**: Added 200+ localized strings and icons (📼, ✨, 📸, etc.) to all properties for better scanability.
- **Stylized Branding**: Updated UI headers with stylized Unicode characters (𝔼𝕞𝕦𝕃𝕖𝕟𝕤𝔽𝕏).
- **Restored Bokeh Shapes**: Re-enabled and fixed the Aperture Shape control, allowing for 3-8 sided polygonal highlights.
- **Improved Handheld Blur**: Upgraded from 1D vertical to high-quality 2D cross-Gaussian defocus.

#### **Bug Fixes & Reliability**
- **Filter Chain Scaling Fix**: Prioritized `gs_get_render_target()` for dimensions, ensuring the plugin respects preceding filters (scaling, cropping).
- **Resize Stability**: Relaxed thrashing detection to handle rapid window resizes without aborting.
- **Initialization Fix**: Ensured output dimensions are explicitly zeroed on sizing failure to prevent GPU driver undefined behavior.

---

## [0.4.2] - 2026-01-26

### 🚀 Major Improvements

#### **Critical Bug Fixes**
- **Fixed Undefined Symbol Error**: Resolved compilation failure by replacing undefined `BLUR_EPSILON` with `BLUR_CACHE_EPSILON`.
- **Restored Plugin Initialization**: Fixed a bug where global initialization ran on every module load; correctly guarded with `plugin_state.initialized`.
- **Eliminated UI Race Condition**: Removed file-static `g_previous_mode` and refactored UI visibility updates to be fully instance-safe, preventing glitches when multiple filters are active.

#### **Logic & Safety**
- **Hardened Mutex Initialization**: Added error checking to `pthread_mutex_init` to prevent unstable states during resource exhaustion.
- **Improved Graphics Robustness**: Added pass count validation before entering technique loops and texture null checks before dimension comparisons.
- **Enhanced Error Recovery**: Optimized `blur_error_fallback` to preserve valid cache data during transient failures.
- **Initialized Output Variables**: Prevented undefined behavior in `lens_get_render_target_size` by providing default dimensions.

#### **Cleanup & Maintenance**
- **Established Defense Strategy**: Created `DEVELOPER_GUIDELINES.md` to document coding standards and prevent regressions.
- **Standardized Naming**: Unified blur-related variable names and removed orphaned settings in handheld strategy.
- **Dead Code Elimination**: Removed unused `missing_params` bitfield and redundant GPU clear operations during initialization.

## [0.4.1] - 2026-01-24

### 🚀 Major Improvements

#### **API Compliance & Robustness**
- **Added explicit OBS version requirement** (>= 28.0.0) to `CMakeLists.txt` for modern API support.
- **Resolved Incorrect Sizing Issue**: Prioritized `gs_get_render_target()` for dimensions, ensuring the plugin respects the preceding filter chain (scaling, cropping, etc.).
- **Improved render target handling** with robust fallback to base dimensions if `gs_get_render_target()` returns NULL.
- **Enhanced filter safety** by gracefully skipping the filter if GPU resource allocation fails, preventing black screens or crashes.
- **Fixed static declaration order** in `lens-core.c` to resolve build warnings/errors.

#### **Lifecycle Management**
- **Verified 100% compliance** with OBS memory allocators (`bmalloc`, `bzalloc`, `bfree`) across all core and strategy files.
- **Validated thread-safety** of UI property modifications and settings caching.

---

## [0.4.0] - 2026-01-18

### 🚀 Major Improvements

#### **Thread Safety & Stability**
- **Fixed unsafe blur cache access** in multi-threaded environments with mutex protection
- **Added comprehensive error recovery** for texture recreation failures
- **Implemented graceful degradation** when GPU resources are unavailable
- **Added overflow protection** for performance counters in long-running streams

#### **Memory Management**
- **Fixed memory leaks** in effect loading error paths using consistent `bfree()`/`bzalloc()`
- **Added parameter validation system** to detect missing shader parameters
- **Improved resource cleanup** in all error paths

#### **Rendering Pipeline**
- **Fixed cropping and resizing issues** by removing `OBS_ALLOW_DIRECT_RENDERING` flag
- **Properly implemented render target dimension handling** - render to fill the entire render target that OBS creates
- **Corrected UV coordinate scaling** based on actual render target dimensions

#### **Code Quality**
- **Extracted magic numbers** to named constants for better maintainability
- **Improved CMake organization** with source file variables
- **Enhanced error messages** with detailed context
- **Added defensive programming** practices throughout

#### **Performance & Monitoring**
- **Added performance counter overflow protection** (resets every 1M frames)
- **Improved blur weight caching** with thread-safe access
- **Added GPU capability detection** for adaptive rendering
- **Enhanced logging** with better error context

### 🛠️ **Technical Changes**

#### **Core Improvements**
- Added `pthread_mutex_t blur_cache_lock` for thread-safe blur operations
- Modified `recreate_texrenders_if_needed()` to return boolean status
- Added `validate_param_availability()` function for shader compatibility checking
- Extracted constants: `TIME_WRAP_THRESHOLD`, `BLUR_EPSILON`, `MAX_BLUR_SAMPLES`, etc.

#### **Build System**
- Organized CMakeLists.txt with `CORE_SOURCES` and `STRATEGY_SOURCES` variables
- Improved installation verification with dedicated script
- Updated version to 0.4.0 reflecting stability improvements

#### **Documentation**
- **Completely rewrote README.md** with comprehensive feature list and installation guide
- **Updated AGENTS.md** with resolved issues section
- **Added installation verification script** (`verify_install.sh`)
- **Enhanced API documentation** and development guidelines

### 🧪 **Testing & Validation**
- **Memory leak tests**: Pass with 0 leaks detected
- **Shader syntax validation**: All techniques and uniforms verified
- **Thread safety**: Blur cache operations now protected
- **Error handling**: Comprehensive validation in all code paths

### 📦 **Installation**
- **Verified user folder installation** to `~/.config/obs-studio/plugins/emuLensFX`
- **Added verification script** to confirm proper installation
- **Improved build instructions** with clear cross-platform guidance

### 🎯 **Compatibility**
- **Backward compatible** with existing OBS installations
- **Cross-platform support** maintained (Linux, Windows, macOS)
- **Shader compatibility** improved with validation system

---

## [0.3.1] - Previous Version
- Initial public release with basic effect modes
- Basic Vidicon, Starburst, Light Leak, Handheld, Bokeh effects
- Fundamental OBS plugin integration

---

**Legend:**
- 🚀 Feature additions
- 🛠️ Technical improvements
- 🧪 Testing enhancements
- 📦 Packaging/installation
- 🎯 Compatibility fixes
---

# Development Notes (from CHANGES.md)

## Summary

Fixed critical OBS API usage issues, improved thread safety, added bloom effect, and ensured proper resource management in the emuLensFX OBS Studio plugin.

## Shader Improvements

### 1. Added Dedicated Bloom/Glow Effect
- **File**: `data/emulens.effect`
- **New Techniques**: `DrawBloomExtract`, `DrawBloomCombine`
- **New Uniforms**: `bloom_threshold`, `bloom_intensity`, `bloom_radius`, `bloom_softness`, `bloom_highlights_only`
- **Impact**: Explicit bloom control separate from Orton effect, better quality, uses existing blur infrastructure
- **Pipeline**: Extract bright areas → Blur (horizontal + vertical) → Combine with original

### 2. Updated Shader Organization
- **Section Numbers**: Updated to include bloom (section 8)
- **Techniques**: 13 total techniques including 2 new bloom techniques

## Issues Fixed

### 1. Thread Safety & Portability
- **File**: `src/plugin-main.c`
- **Issue**: Direct `pthread_*` usage for global plugin state
- **Fix**: Replaced with libobs atomic operations (`os_atomic_*`) for cross-platform portability

### 2. Render-Target Lifetime Race Condition
- **File**: `src/lens-core.c`
- **Issue**: Texrender resizing was happening in `video_tick()` (different thread context)
- **Fix**: Deferred resize requests to render thread via atomic flags

### 3. Technique Begin/End Pairing
- **File**: `src/lens-core.c`, strategy files
- **Issue**: `gs_technique_begin()` was not always paired with `gs_technique_end()` in error paths
- **Fix**: Added helper functions that ensure proper pairing

### 4. Dangling Fallback Texture Pointer
- **File**: `src/lens-core.c`
- **Issue**: `last_valid_blur_result` pointer wasn't cleared after texrender destruction
- **Fix**: Clear pointer before texrender destruction

### 5. Matrix Stack Isolation
- **File**: `src/lens-core.c`
- **Issue**: Custom draw paths mixed OBS filter helpers with manual matrix operations
- **Fix**: Isolated matrix stack usage, using `gs_matrix_push()`/`gs_matrix_pop()` around manual draw operations

## Strategy File Updates

All strategy files were updated to use the new safe technique helper functions.

## Testing

All tests pass with the changes applied:
- **ShaderSyntaxTest**: Passed
- **MemLeakTest**: Passed
- **PythonIntegrationTest**: Skipped (obspython not available, expected behavior)
