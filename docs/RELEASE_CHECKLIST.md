# EmuLens — public preview verification

This repository contains a clean source snapshot. Its public Git history is intentionally separate from the private development repository. This checklist documents test boundaries, not cross-platform runtime certification.

## Verified public source checkpoint — 2026-11-09

- Public repository:
  [myrqyry/emuLens](https://github.com/myrqyry/emuLens), root
  commit `354c87ac407e29046715b362dfd774d80f7ea82c`.
- [Main GitHub CI](https://github.com/myrqyry/emuLens/actions/runs/42732888596)
  succeeded across all five jobs: formatting, Python/installer tests,
  Linux native/GPU tests and source packaging, Windows and macOS builds.
- [OBS 33.0.1 compatibility](https://github.com/myrqyry/emuLens/actions/runs/42732888818)
  passed on Ubuntu 26.04.
- Linux tests: 51 registered, 50 passed, one optional OBS-Python
  integration skipped. Six additional installer-safety tests passed.
- Downloaded and inspected all three public CI packages. Linux contains
  the ELF `emuLensFX.so`; Windows contains DLLs; macOS contains
  an unsigned `emuLensFX.plugin`. Each includes the shader and
  61 presets. The downloaded CPack source archive had 203 verified
  members with no private diaries or agent files.
- Not yet qualified: full OBS GUI interactions and long-session streaming
  performance, plus installed Windows/macOS runtime behavior.

## Source and privacy

- Public source begins from one reviewed snapshot, not the full development history.
- Internal diaries, agent instructions, local Meristem state, credentials and generated files are excluded.
- CI source archives must be checked for private paths before upload.
- GPL-2.0-or-later licensing is included in LICENSE.

## Build and runtime qualification

- OBS 32.1.0 or newer is required. A plugin built against a newer libobs SDK may not load on an older OBS.
- Linux CI builds against the pinned OBS SDK, runs its real CTest suite including GPU-pixel tests, and stages the module, shader and 61 presets.
- Windows and macOS CI compile packages; this is not proof that the plugin loads or renders correctly on a real OBS installation on those platforms.
- Isolated GPU rendering tests are not sustained stream-performance measurements.
- Do not install tests into the everyday OBS profile or alter saved scenes.
- No tagged signed release or OBS plugin-directory listing is implied by publishing source.

## Local verification

    EMULENS_TEST_BUILD_DIR=/tmp/emulensfx-check ./tests/run_tests.sh
    python3 -m unittest discover -s tests -p test_safe_installer.py -v
    cmake -S . -B /tmp/emulensfx-build -Dlibobs_DIR=/path/to/libobs/cmake
    cmake --build /tmp/emulensfx-build
    cmake --install /tmp/emulensfx-build --prefix /tmp/emulensfx-stage
    cmake --build /tmp/emulensfx-build --target package_source
    python3 tests/check_source_archive.py release/emuLensFX-0.5.0-source.tar.xz

CPack archives the working tree rather than only Git-tracked files. Keep source archive exclusions even when .gitignore already covers private files.
