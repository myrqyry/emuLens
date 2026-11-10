# EmuLens

GPU-accelerated lens, optical, camera, and film-emulation effects for OBS Studio.
Use EmuLens as a source filter to build everything from restrained optical
imperfections to deliberately stylized analog looks without leaving the OBS
graphics pipeline.

**Public preview source — v0.5.0 (experimental).** All 17 effects, including Wet Lens and Phone Wet Lens, are in this snapshot. Linux has passed isolated OBS/OpenGL tests. Windows and macOS compile/package success does not yet establish that the plugin loads and renders correctly on those platforms. See [CHANGELOG.md](CHANGELOG.md) and [release testing boundaries](docs/RELEASE_CHECKLIST.md).

**Compatibility:** EmuLens is the public project and OBS display name. The internal OBS source ID emulens_fx, binary name emuLensFX, shader resource paths, and installer layout intentionally retain their original names so saved scenes and installations remain compatible. Do not rename the plugin binary or installation directory manually.

## Features

- **17 effect modes** — Vidicon, Starburst, Light Leak, Handheld, Bokeh, Particles,
  Orton, Film Grit, Mirage, Center Spot, Bloom/Glow, Pinhole, Polaroid,
  Daguerreotype, Technicolor 2-Strip, Wet Lens (water droplets), and
  Phone Wet Lens (compact-camera water artifacts).
- **61 preset files** — 17 restrained defaults plus 44 alternate looks.
- **Optics-first behavior** — temporal pickup-tube persistence, highlight-driven
  diffraction, fixed lens contamination, center-spot optical rings, aperture
  bokeh, mirrored kaleidoscopic sampling, and size-aware particles.
- **GPU-native rendering** — persistent OBS GPU resources, multipass texrender
  pipelines where required, and per-instance state.
- **Filter-chain aware** — renders against the current OBS filter target and
  respects upstream filters such as scaling and cropping.

## Verified GitHub Actions builds

The public [CI workflow](https://github.com/myrqyry/emuLens/actions/workflows/ci.yml)
builds Linux x86_64, Windows x64 and macOS packages and runs formatting,
Python checks, installer-safety tests and Linux native/GPU render tests.
A separate [OBS 33 compatibility workflow](https://github.com/myrqyry/emuLens/actions/workflows/obs33-compatibility.yml)
checks an OBS 33.0.1 build.

For the public snapshot `354c87a` (November 9, 2026), both
workflows passed:

- [Main CI: all five jobs passed](https://github.com/myrqyry/emuLens/actions/runs/42732888596).
  Downloadable artifacts: `emulensfx-linux-obs32`,
  `emulensfx-windows-obs32` and `emulensfx-macos-obs32`.
- [OBS 33.0.1 compatibility: passed](https://github.com/myrqyry/emuLens/actions/runs/42732888818).

The packages contain the platform-specific module/bundle,
`emulens.effect` and all 61 presets; the Linux package also includes
localization. The downloaded source archive passed the private-file
exclusion checks. GitHub retains workflow artifacts for a limited time.
There is no tagged or signed GitHub Release yet.

**Compatibility:** OBS 32.1.0+ is required, but a module compiled
against a newer libobs SDK may not load in an older OBS installation.
Windows and macOS packaging success does not establish runtime loading
or visual correctness on those platforms.

## Installation

OBS should be closed while replacing the plugin binary. Restart OBS after an
install so the shared module and shader data are reloaded together.

### Cross-platform wrapper

On Linux and macOS:

```bash
./build.sh
```

`build.sh` dispatches to the native installer. In a Windows-like shell it
prints the PowerShell command to use instead of attempting a Unix install.

### Linux

```bash
./install_local.sh
```

Installs to:

```text
~/.config/obs-studio/plugins/emuLensFX
```

The installer supports explicit libobs CMake discovery overrides:

```bash
OBS_CMAKE_DIR=/path/to/libobs/cmake ./install_local.sh
# or
libobs_DIR=/path/to/libobs/cmake ./install_local.sh
```

It stages the CMake install into a temporary prefix, then copies the real module
and data tree into the OBS user-plugin layout.

Verify a Linux user install with:

```bash
./verify_install.sh
```

The verification script checks the plugin directory, shared module, shader,
preset directory, and locale directory. For release candidates, verify the staged binary and accompanying resources **without installing over a running user's plugin**. CPack source packaging does not honor Git ignore; the [release checklist](docs/RELEASE_CHECKLIST.md) includes mandatory source-archive privacy checks.

### macOS

```bash
./install_local_macos.sh
```

Installs to:

```text
~/Library/Application Support/obs-studio/plugins/emuLensFX
```

Choose a CMake configuration through `BUILD_TYPE`:

```bash
BUILD_TYPE=RelWithDebInfo ./install_local_macos.sh
```

When Homebrew OBS is present, the installer supplies its prefix to CMake
automatically. The installer defaults to `Release`; `BUILD_TYPE=RelWithDebInfo` is optional.

### Windows

Run from PowerShell:

```powershell
.\install_local.ps1
```

The installer defaults to `Release`. Supported values are `Debug`,
`Release`, `RelWithDebInfo`, and `MinSizeRel`:

```powershell
.\install_local.ps1 -BuildType RelWithDebInfo
```

The user plugin is installed under
`%APPDATA%\obs-studio\plugins\emuLensFX`.

The installers choose the user-level paths shown above; a custom prefix
is not needed for a default install. On Linux, `install_local.sh`
builds into a disposable stage, validates the binary/effect/locale and
prepares a rollback-safe replacement. Windows and macOS installation
still needs runtime verification on those platforms. Restart OBS afterward.

## Usage

1. Add a video or image source to a scene.
2. Right-click the source → **Filters**.
3. Click **+** → **EmuLens**.
4. Choose an **Effect Type**.
5. Start from a preset or tune the visible controls directly.

### Effects

| Effect | Current behavior | Preset examples |
|---|---|---|
| **Vidicon (Trails)** | Pickup-tube image persistence with temporal RGB lag, red-dominant retention, optional comet/smear artifacts, tube noise, scanlines, registration drift, halation, and hum | `vidicon_dream`, `vidicon_soft` |
| **Starburst** | Diffraction stars from point/specular highlights; brighter highlights produce stronger and longer rays | `starburst_anamorphic`, `starburst_cinema` |
| **Film Burn / Leak** | Procedural edge-biased light leaks with color shifting, directional bias, streaking, complexity, and multiple blend modes | `leak_warm`, `leak_chaos` |
| **Handheld Camera** | Position/rotation drift, breathing, tremor, step bob, inertia, correction motion, and optional blur | `handheld_run`, `handheld_breathing` |
| **Bokeh / Tilt-Shift** | Shallow depth-of-field with configurable focus region, highlight response, and 3–8 sided aperture shapes | `bokeh_neon`, `bokeh_tiltshift` |
| **Particles** | Floating dust, snow, and motes with source influence, focus shaping, polygonal particles, chromatic separation, and size-aware energy | `part_snow`, `part_magic` |
| **Orton Glow** | Soft-focus photographic glow with blur, contrast, brightness, saturation, highlight boost, and blend modes | `orton_angelic`, `orton_velvet` |
| **Film Grit** | Film-stock dirt, scratches, grain, flashes, instability, tone controls, scratch types, and blend modes | `grit_grindhouse`, `grit_1920s` |
| **Mirage** | Layered mirrored/kaleidoscopic image echoes; large radius values reflect at image bounds instead of dropping samples to black | `mirage_split`, `mirage_drunk` |
| **Center Spot** | Clear center with an optical outer ring: Soft Diffusion, Diopter, or Radial Zoom | `spot_mist`, `spot_vignette`, `spot_tunnel` |
| **Bloom / Glow** | Multi-level bright-pass bloom with optional fixed lens contamination that scatters, veils, glints, refracts, ghosts, and supports anamorphic response | `bloom_dusty_lens`, `bloom_prism` |
| **Pinhole** | Soft lensless-camera rendering with vignette, distortion, sepia, and grain | `pinhole_classic`, `pinhole_antique` |
| **Polaroid** | Instant-film color response with softness, vignette, emulsion shift, development, grain, and bleed | `polaroid_expired`, `polaroid_vintage` |
| **Daguerreotype** | Silver-plate contrast, grain, tint, vignette, and edge softness | `daguerre_silver`, `daguerre_tarnished` |
| **Technicolor 2-Strip** | Restricted red/cyan dye-transfer character with saturation, strip offset, and glow | `technicolor_classic`, `technicolor_misaligned` |
| **Wet Lens** | Stationary water-bead refraction of captured live pixels, with light-gathering glints | `default_wet_lens`, `wet_lens_night_window` |
| **Phone Wet Lens** | Broad near-lens refraction regions, captured-light ribbons and veiling glare | `default_phone_wet`, `phone_wet_streetlights` |

Every mode has an explicit `default_<mode>.json` preset whose visible values
match that strategy's runtime defaults.

## Notable controls and optical behavior

### Vidicon Trail Decay

**Trail Decay** is a perceptual persistence control rather than a raw per-frame
feedback coefficient. Persistence is time-normalized so the trail duration is
stable across frame rates.

- `0.50` — short but clearly visible persistence
- `0.80` — natural/default tube persistence
- `0.90` — longer trail
- `0.95` — long trail
- `0.99` — very long but still decaying
- `1.00` — intentional indefinite hold

The feedback buffer stores trail energy rather than repeatedly re-exposing the
whole source frame. Per-channel temporal lag allows red to persist longer than
blue without faking channel lag through spatial offsets.

Scene collections carrying the recognizable dead legacy Vidicon combination
are migrated to the current tube defaults. Ordinary customized Vidicon
settings are not reset.

### Bloom / Dusty Lens

Bloom uses a bright-pass extraction, multilevel downsample/upsample pyramid,
and final combine stage. Its lens-contamination controls model light
interacting with the optical surface rather than a dirt texture painted over
the frame:

- **Dust Amount** — amount of fixed contamination revealed by captured light.
- **Dust Scale** — size of the grime/smudge structure.
- **Dust Contrast** — definition of the contamination pattern.
- **Veiling Glare** — broad light scatter through fine contamination.
- **Dust Specular Glint** — sharper glints from lit contamination.
- **Lens Surface Layers** — number of fixed dirt/smudge scales used to build
  the optical-surface pattern.
- **Refraction Amount / Refraction Scale** — chromatic/light displacement driven
  by the local gradient of the contamination pattern.
- **Ghost Amount / Scale / Color** — internal lens reflections.
- **Anamorphic** — stretched optical response for streak-like bloom.
- **Highlights Only** — restricts the combine toward extracted highlights.

With no extracted highlight energy, the contamination response stays dormant.
The pattern is fixed in lens space rather than drifting like particles.
Large-radius pyramid sampling treats samples outside the image as black instead
of replicating frame edges.

Bloom is a multipass strategy: mode-specific uniforms are rebound after common
parameter binding for each relevant pass so live UI changes reach extraction,
blur/pyramid, and combine techniques.

### Center Spot

The **Outer Optic** selector chooses how the ring surrounding the clear center
is constructed:

- **Soft Diffusion**
- **Diopter**
- **Radial Zoom**

The inner spot remains genuinely clear; blur/zoom behavior belongs to the outer
annulus.

### Starburst

Starburst ray reach is driven by highlight energy. Strong point highlights
produce longer/brighter rays while weaker highlights fall off instead of being
allowed to smear farther through a distance-lowered threshold. Point count,
rotation, ray length, thickness, smoothness, chromatic aberration, pulse,
anamorphic mode, colorization, samples, and blend mode remain independently
tunable.

### Mirage

Mirage uses mirrored boundary sampling at large **Radius** values. Out-of-frame
coordinates fold back into the image, so large displacements remain populated
instead of producing black wedges from zero valid samples.

### Particles

Large particles compensate their energy as size increases and use bounded
screen-style compositing. Dense overlaps therefore retain visible particle
structure instead of turning into opaque white blobs.

### Water-on-lens effects

**Wet Lens** uses stationary, seed-deterministic water beads to refract
the live source image. Captured highlights produce rim glints and subtle
chromatic separation. Coverage, bead size/stretch, refraction, light
response, and pattern seed are adjustable; **Clean Raindrops** and
**Night Window** provide starting points. Zero coverage leaves the frame
unchanged. It does not simulate flowing or merging droplets.

**Phone Wet Lens** uses broader, irregular wet-surface regions with
refracted imagery, illuminated contours, veiling glare, and directional
light ribbons derived from actual source highlights. Controls include
coverage, patch scale, contour strength, refraction, ribbon strength/reach,
glow, angle, and pattern. Presets include **Wet Phone Camera**,
**Rain on Streetlights**, and **Heavy Wet Optics**. The pattern remains
stationary; a dark source cannot generate bright ribbons on its own.

Both modes have isolated OBS/OpenGL pixel tests, including refraction,
light response, pattern stability, and alpha handling. These do not
establish real-time performance for every live scene or GPU.

## Building

### Prerequisites

- OBS Studio development headers (`32.1.0+`)
- CMake `3.28+`
- C17-compatible compiler
- Python 3 for contract tests
- Ninja for the shipped Ubuntu CMake preset
- Visual Studio 2022 for the shipped Windows x64 preset
- Xcode for the shipped macOS preset

### Direct CMake build

```bash
cmake -S . -B build
cmake --build build
```

For local installation, prefer the platform installer so the module/data layout
matches what OBS expects.

### CMake presets

The repository ships configure/build presets for the supported CI/development
targets:

```bash
# Linux
cmake --preset ubuntu-x86_64
cmake --build --preset ubuntu-x86_64

# macOS
cmake --preset macos
cmake --build --preset macos

# Windows
cmake --preset windows-x64
cmake --build --preset windows-x64
```

CI variants are available as `ubuntu-ci-x86_64`, `macos-ci`, and
`windows-ci-x64`.

## Tests

The canonical local test entry point is:

```bash
./tests/run_tests.sh
```

It configures the standalone test project, builds it, runs CTest, and then runs
the optional OBS Python integration test when `obspython` is available.

Choose a different test build directory with:

```bash
EMULENS_TEST_BUILD_DIR=/tmp/emulens-tests ./tests/run_tests.sh
```

Equivalent manual commands are:

```bash
cmake -S . -B tests/build -DENABLE_TESTING=ON
cmake --build tests/build --parallel "$(nproc)"
ctest --test-dir tests/build/tests --output-on-failure
```

Focused Python contract tests can run without the OBS Python runtime, for
example:

```bash
python3 tests/test_shader_syntax.py
python3 tests/test_render_pipeline.py
python3 tests/test_gpu_resources.py
python3 tests/test_bloom_lens_effect.py
python3 tests/test_intended_effects_recovery.py
python3 tests/test_mirage_radius_bounds.py
python3 tests/test_vidicon_reference_model.py
```

The public v0.5.0 snapshot registers **51 CTest entries**. On the
reviewed Linux environment **50 passed**, including the real Wet Lens
and Phone Wet Lens GPU render tests. The optional `PythonIntegrationTest`
skipped because `obspython` was unavailable. These tests do not establish
sustained stream performance or full GUI behavior. All six installer-safety
regression tests also passed independently.

## Architecture

EmuLens uses a strategy-per-effect architecture.

- **Core rendering** (`src/lens-core.c`) — filter lifecycle, settings snapshots,
  shared GPU resources, render dispatch, dimensions, and final composite.
- **OBS helpers** (`src/obs-utils.c`) — upstream capture, common parameter
  binding, shader loading, render helpers, and settings rebinding.
- **Effect strategies** (`src/strategies/`) — one file per effect mode.
- **UI** (`src/lens-ui.c`) — dynamic OBS properties, mode visibility, preset
  selection, and control ranges.
- **Shaders** (`data/emulens.effect`) — effect uniforms and the 22 cached
  techniques used by the strategies.
- **Presets** (`data/presets/`) — 61 JSON files: 17 defaults + 44 alternates.
- **Localization** (`data/locale/`) — property labels, tooltips, mode names,
  preset names, and descriptions.
- **Tests** (`tests/`) — shader/static contracts, reference models, resource
  lifecycle checks, C++ strategy tests, and optional OBS Python integration.

## OBS/runtime contract

The plugin follows the OBS source/filter lifecycle rather than treating module
loading or compilation as proof that an effect works.

- GPU resources are created and destroyed only with a valid OBS graphics
  context.
- The current filter target is used inside the render lifecycle; recursive
  source-size callbacks are not used for filter dimensions.
- Common parameter binding can disturb mode-specific effect state on some
  backends, so strategies that need it rebind current settings immediately
  afterward. Bloom does this across its multipass pipeline.
- Settings snapshots and render work use bounded synchronization; render
  callbacks avoid filesystem/network I/O and blocking waits.
- Persistent texrenders are reused rather than allocated per frame.
- Expected per-frame rendering is silent. Logging in hot render paths is
  reserved for actionable failures rather than entry/exit traces.
- Runtime validation instantiates the real filter and shader in OBS and checks
  rendered output, not just module loading.

The current runtime harness exercises OBS Studio 32.2.0 on Linux/OpenGL,
including Vidicon temporal persistence, the Trail Decay curve, oversized
particles, Mirage at large radius, Center Spot optical families,
highlight-driven Starburst, large-radius bloom, Dusty Lens light response, and
fixed-in-lens-space contamination.

## License

EmuLens is licensed under the GNU General Public License version 2 or later
(`GPL-2.0-or-later`), matching OBS Studio's licensing. See [LICENSE](LICENSE).

## Credits

Developed for the OBS Studio community.
