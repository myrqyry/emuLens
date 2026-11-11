<!-- meristem-template:v1 -->
# Project

## Purpose

EmuLens is an OBS Studio source filter providing GPU-accelerated lens, optical,
camera-motion, and film-emulation effects within OBS's graphics pipeline. Its
distinctive premise is optics-first behavior that ranges from restrained lens
imperfections to intentionally stylized analog looks.

## Invariants

- Preserve the internal OBS source ID `emulens_fx`, binary name `emuLensFX`,
  shader resource paths, and installer layout for saved-scene and installation
  compatibility; the public/display name is EmuLens.
- OBS 32.1.0 or newer is required; a module built against a newer libobs SDK may
  not load on an older OBS installation.
- Each effect's `default_<mode>.json` preset is expected to match its runtime
  defaults. Rendering uses OBS GPU resources and filter-chain targets, including
  upstream scaling/cropping filters.

## Architecture and boundaries

- C++/CMake OBS plugin and GPU effects: `src/`, with shipped shaders, presets,
  and localization under `data/`.
- Build, install, and platform packaging: root `CMakeLists.txt`, `cmake/`,
  `build.sh`, and platform installer scripts.
- Tests: CMake/CTest native and GPU tests plus Python contract and installer
  checks under `tests/`.
- `README.md` is the user-facing feature/install/compatibility authority;
  `docs/RELEASE_CHECKLIST.md` owns release qualification and verification
  boundaries. `CHANGELOG.md` records published changes.

## Known-good workflows

See `README.md` for installation and `docs/RELEASE_CHECKLIST.md` for the
bounded release-verification commands. The documented local checks include:

- `EMULENS_TEST_BUILD_DIR=/tmp/emulensfx-check ./tests/run_tests.sh`
- `python3 -m unittest discover -s tests -p test_safe_installer.py -v`
- CMake configure/build/install and `package_source` checks (see release
  checklist for exact commands).

## Verification

Use the CTest/native and GPU suite plus Python installer/contract checks for
local changes; use the release checklist's source-archive inspection before
publishing source packages. CI builds Linux, Windows, and macOS, but compile or
package success on Windows/macOS is not runtime-load/render qualification.

## Open structural uncertainty

Windows and macOS plugin load/render behavior on real OBS installations, full
OBS GUI interaction, and long-session streaming performance are not qualified
by the documented CI checks. See `docs/RELEASE_CHECKLIST.md`; this is a known
release boundary, not an assertion that those behaviors fail.
